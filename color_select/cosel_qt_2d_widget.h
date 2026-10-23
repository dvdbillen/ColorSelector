// SPDX-License-Identifier: MIT
// Copyright (c) 2026 David Billen

// 2D HSV color control

#ifndef cosel_qt_2d_widget_h
#define cosel_qt_2d_widget_h

#include <QBoxLayout>
#include <QFont>
#include <QImage>
#include <QMouseEvent>
#include <QPainter>
#include <QPixmap>
#include <QPoint>
#include <QRadioButton>
#include <QRect>
#include <QWidget>

#include <memory>

#include "cosel_gradient.h"
#include "cosel_model.h"
#include "cosel_model_hsx.h"
#include "cosel_qt_app_defs.h"
#include "cosel_qt_gradient_slider.h"
#include "cosel_qt_selected_color.h"
#include "cosel_qt_throttled.h"
#include "cosel_qt_tools.h"

namespace cosel {

namespace details {

// This flag causes the gradient to vary with saturation.
//
// Reasons to set true:
//      - A little useful
//      - More consistent with how gradients are handled elsewhere
//
// Reasons to set false:
//      - Adds processing when the saturation changes (might be relevant at resolution==1)
//      - Only truly accurate for the top row

constexpr bool vary_gradient_with_sat() {
    return true;
}

using hsv_t = hsv2;
using grad_method_t = hsx_hue_gradient_t<hsv_t>;
using grad_t = std::shared_ptr<grad_method_t>;

// Limit the S and V ranges that can actually be handled by the control. This avoids a number of
// issues coping with gray and also pure red that has noise in the blue component.
constexpr double hsv_2d_vmin = 1.0/255*8;
constexpr double hsv_2d_smin = 1.0/255*8;


////////////////////////////////////////////////////////////////////////////////////////////////////
// Class that renders the 2D pixmap. The render_image function makes it a duck type that is used
// with a throttled_pixmap class handled by selector_2d

struct render_2d {
    // s is the hsv 's' level
    explicit render_2d(double s, int width, int height, int resolution, bool bordered, grad_t grad_) :
        s_(s),
        width_(width),
        height_(height),
        resolution_(resolution),
        border_((bordered && resolution >= 4)? 1: 0),
        grad_(std::move(grad_)) {}

    render_2d() = default;

    void render_image(QImage& image) const noexcept {
        if(!grad_) {
            image = QImage();
            return;
        }

        const auto rez = resolution_;

        // Reallocate the image if needed
        if(image.isNull() || image.width() != width_ || image.height() != height_) {
            image = QImage(width_, height_, QImage::Format_RGBA8888);
            image.fill(qRgba(0, 0, 0, 0));
        }

        // Setup rendering
        auto* pixel_base = reinterpret_cast<uint32_t*>(image.bits());
        const auto word_pitch = image.bytesPerLine() / 4;

        const auto xinc = 1.0/(width_);
        const auto yinc = 1.0/(height_);

        if(rez < 2) {
            // Handle rez == 1 uniquely because it's cleaner and more performant
            for(int y = 0; y < height_; ++y) {
                const auto v = lerp(hsv_2d_vmin, 1.0, 1 - y*yinc);

                auto* NOALIAS pixels = &pixel_base[y*word_pitch];

                for(int x = 0; x < width_; ++x) {
                    const auto h = grad_->gradient_to_source(x*xinc);

                    const auto rgb_c = hsv_t(h, s_, v).to_rgb();
                    pixels[x] = to_image_pixel(rgb_c.clamped_gamut(), 255);
                }
            }
        } else {
            // Rez > 1
            for(int y = 0; y < height_; y += rez) {
                const auto v = lerp(hsv_2d_vmin, 1.0, 1 - y*yinc);

                auto* NOALIAS pixels = &pixel_base[y*word_pitch];

                for(int x = 0; x < width_; x += rez) {
                    const auto h = grad_->gradient_to_source(x*xinc);

                    const auto rgb_c = hsv_t(h, s_, v).to_rgb();
                    const auto pixel = to_image_pixel(rgb_c.clamped_gamut(), 255);

                    const auto length = std::min(rez, width_ - x) - border_;

                    // Propagate color to rez columns
                    for(int i = 0; i < length; ++i) {
                        pixels[x + i] = pixel;
                    }
                }

                const auto length = std::min(rez, height_ - y) - border_;

                // Propagate scanline to rez rows
                for(int i = 1; i < length; ++i) {
                    memcpy(&pixel_base[(y + i)*word_pitch], pixels, width_*4);
                }
            }
        }
    }

    void set_s(double s) {
        s_ = s;
    }

    void set_grad(const grad_t& grad) {
        grad_ = grad;
    }

    double s() const {
        return s_;
    }

    int width() const {
        return width_;
    }

    int height() const {
        return height_;
    }

    int resolution() const {
        return resolution_;
    }

    bool bordered() const {
        return border_ != 0;
    }

    const grad_t& grad() const {
        return grad_;
    }

private:
    double s_ = .5;
    int width_ = 24;
    int height_ = 24;
    int resolution_ = konst::default_2d_resolution();
    int border_ = konst::default_2d_bordered()? 1: 0;
    grad_t grad_;
};


////////////////////////////////////////////////////////////////////////////////////////////////////
// Widget that shows the 2D colors and allows selection. The parent of this is the one used by the
// application. It is defined below and adds the saturation setting slider.

class selector_2d : public QWidget {
    Q_OBJECT;

public:
    explicit selector_2d(selected_color_t color_,
                         int resolution_ = konst::default_2d_resolution(),
                         bool bordered_ = konst::default_2d_bordered()) :
        selection(std::move(color_)),
        image_renderer(this, false),
        grad(std::make_shared<hsx_hue_gradient_t<hsv_t>>()),
        resolution(resolution_),
        bordered(bordered_) {

        color_id_ = selection->assign_id();
        current = hsv_t(selection->rgb());
        connect(selection.get(), &selected_color::changed, this, [this](unsigned id) {
            if(isVisible() && id != color_id_) {
                rethink();
            }
        });

        std::tie(bg_color, fg_color) = get_bg_fg_colors(this);

        // Need a minimum size else the rendering fails
        setMinimumSize(resolution_*2, resolution_*2);
    }

    double s() const {
        return image_renderer.info().s();
    }

    unsigned color_id() const {
        return color_id_;
    }

    // Cause the image_renderer to be updated as well as the widget
    void rethink() {
        current = hsv_t(selection->rgb());

        const auto s = current.s();
        const auto wr = get_work_rect();
        const auto w = wr.width();
        const auto h = wr.height();
        const auto grad_size = w;

        if constexpr(details::vary_gradient_with_sat()) {
            // The gradient varies with saturation so a new one needs to be generated in all cases

            if(grad->size() != unsigned(grad_size)) {
                grad = std::make_shared<hsx_hue_gradient_t<hsv_t>>(s, 1, size_t(grad_size));
            } else {
                grad->set(s, 1, grad->size());
            }
        } else {
            // The gradient does not vary with saturation so a new one only needs to be generated
            // if the size changed

            if(grad->size() != unsigned(grad_size)) {
                grad = std::make_shared<hsx_hue_gradient_t<hsv_t>>(1, 1, size_t(grad_size));
            }
        }

        const auto& old = image_renderer.info();

        if(old.s() != s || old.width() != w || old.height() != h || old.grad().get() != grad.get()) {
            image_renderer.update(render_2d{s, w, h, resolution, bordered, grad});
        }
    }

private:

    // Quantize an integer by the resolution
    int quantize(int n) const {
        n /= resolution;
        return n*resolution;
    }

    using pointf = point<double>;

    selected_color_t selection;
    unsigned color_id_ = 0;
    hsv_t current;

    QColor bg_color;
    QColor fg_color;

    throttled_image<render_2d> image_renderer;
    details::grad_t grad;
    int resolution = konst::default_2d_resolution();
    bool bordered = konst::default_2d_bordered();

    // The existence of the pixmap determines if valid to render.
    bool valid() const {
        return !image_renderer.pixmap().isNull();
    }

    // All positioning is based off this working rectangle. It forces the width and height to be
    // multiples of the resolution.
    QRect get_work_rect() const {
        const auto w = quantize(width());
        const auto h = quantize(height());
        const auto hmargin = (width() - w)/2;
        const auto vmargin = (height() - h)/2;

        return QRect{hmargin, vmargin, w, h};
    }

    // Convert a Qt QPoint to a normal within the work rect.
    pointf pos_to_normal(const QRect& wr, const QPoint& pos) const {
        const auto workpos = QPoint(pos.x(), pos.y()) - wr.topLeft();
        const auto w = wr.width() - 1;
        const auto h = wr.height() - 1;
        const auto x = quantize(std::clamp(workpos.x(), 0, w));
        const auto y = quantize(std::clamp(workpos.y(), 0, h));

        return pointf(double(x)/w, double(y)/h);
    }

    // Convert a normal point to a Qt point
    QPoint normal_to_pos(const QRect& wr, const pointf& normal_pos) const {
        const auto clamped = pointf(clamp(normal_pos.x()), clamp(normal_pos.y()));
        const auto w = wr.width() - 1;
        const auto h = wr.height() - 1;
        const auto x = clamped.x()*w;
        const auto y = clamped.y()*h;
        const auto qx = quantize(int(x + .5));
        const auto qy = quantize(int(y + .5));

        return QPoint(qx, qy) + wr.topLeft();
    }

    // Find the color at a specific Qt QPoint
    hsv_t pos_to_color(double s, const QRect& wr, const QPoint& pos) const {
        const auto normal = pos_to_normal(wr, pos);
        const auto h = grad->gradient_to_source(normal.x());
        const auto v = lerp(1.0, hsv_2d_vmin, normal.y());
        return hsv_t(h, s, v);
    }

    // Find the Qt QPoint of a specified color
    QPoint color_to_pos(const hsv_t& color, const QRect& wr) const {
        const auto x = grad->source_to_gradient(color.h());
        const auto y = unlerp(1.0, hsv_2d_vmin, color.v());
        return normal_to_pos(wr, pointf(x, y));
    }

    void paintEvent(QPaintEvent* event) override;

    void mousePressEvent(QMouseEvent* event) override {
        mouseMoveEvent(event);
    }

    void mouseMoveEvent(QMouseEvent* event) override {
        if(!grad || grad->empty()) {
            return;
        }

        const auto wrect = get_work_rect();
        const auto s = std::max(hsv_2d_smin, current.s());
        current = pos_to_color(s, wrect, event->pos());
        const auto rgb_c = current.to_rgb();
        selection->set_rgb(rgb_c, color_id_);
        update();
    }

    void resizeEvent([[maybe_unused]] QResizeEvent* event) override {
        rethink();
    }

    void showEvent([[maybe_unused]] QShowEvent* event) override {
        image_renderer.start();
        rethink();
    }

    void hideEvent([[maybe_unused]] QHideEvent* event) override {
        image_renderer.stop();
    }
};

inline void selector_2d::paintEvent([[maybe_unused]] QPaintEvent* event) {
    const auto wrect = get_work_rect();

    if(!isEnabled() || grad->empty()) {
        return;
    }

    QPainter p(this);

    if(!valid()) {
        p.fillRect(QRect(0, 0, width(), height()), bg_color);
        return;
    }

    // Render pixmap

    p.drawPixmap(wrect.x(), wrect.y(), image_renderer.pixmap());

    // Render position marker

    constexpr qreal xy_mark_r = 3;
    const auto xy_pos = color_to_pos(current, wrect) + QPoint(resolution/2, resolution/2);
    const auto xy_rect = QRectF(xy_pos - QPointF(xy_mark_r, xy_mark_r),
                                QSizeF(xy_mark_r*2, xy_mark_r*2));

    p.setRenderHint(QPainter::Antialiasing);

    p.setPen(QColor(255, 255, 255));
    p.drawEllipse(xy_rect);

    p.setPen(QColor(0, 0, 0));
    p.drawEllipse(xy_rect.adjusted(-1, -1, 2, 2));
}


} // namespace details


///////////////////////////////////////////////////////////////////////////////////////////////////
// This class is a parent to the selector_2d and is the one used by the application. It pairs the
// selector_2d with a saturation slider.

class twod_selector : public QWidget {
public:
    explicit twod_selector(const selected_color_t& color_, double corner_rounding,
                                int resolution, bool bordered);

private:
    void showEvent([[maybe_unused]] QShowEvent* event) override {
        handle_color_change(-1);
    }

    details::selector_2d* selector = nullptr;

    selected_color_t selection;

    class sat_gradient_t : public linear_gradient_t<details::hsv_t> {
    public:
        explicit sat_gradient_t(const details::hsv_t& c_, size_t n) :
            linear_gradient_t<details::hsv_t>(c_, n, [](const details::hsv_t& c, double pos) {
            return c.with_s(pos).to_rgb();
        }) {}

    };

    gradient_slider* sat = nullptr;
    std::shared_ptr<sat_gradient_t> sat_gradient;

    void handle_color_change(unsigned id) {
        hsv_color = details::hsv_t(selection->rgb());
        sat->set_value(hsv_color.s()*100);

        sat_gradient->set_color(details::hsv_t(hsv_color.h(), 1, hsv_color.v()));
        sat->set_gradient(sat_gradient);

        if(id != selector->color_id() && selector->s() != hsv_color.s()) {
            selector->rethink();
        }
    }

    void handle_slider_change() {
        const auto l = sat->value();
        hsv_color.set_s(l/100);
        selection->set_rgb(hsv_color.to_rgb(), color_id);
    }

    details::hsv_t hsv_color;
    unsigned color_id = 0;
};

inline twod_selector::twod_selector(const selected_color_t& color_,
        double corner_rounding, int resolution, bool bordered) :
    selection(color_) {

    color_id = selection->assign_id();

    resolution = std::clamp(resolution, 1, 32);
    selector = new details::selector_2d(color_, resolution, bordered);
    selector->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    hsv_color = details::hsv_t(selection->rgb());

    auto* sgrad = new sat_gradient_t(details::hsv_t(1, 1, 1), 50);
    sat_gradient = std::shared_ptr<sat_gradient_t>(sgrad);

    gradient_slider::style_options_t options;
    options.height = konst::slider_2d_height();
    options.rounding = corner_rounding;
    options.info = false;
    sat = new gradient_slider(QString(), 1, 100, options, hsv_color.v()*100);
    sat->set_gradient(sat_gradient);
    sat->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    connect(sat, &gradient_slider::changed, this, [this]() {
        handle_slider_change();
    });

    connect(selection.get(), &selected_color::changed, this, [this](unsigned id) {
        if(isVisible() && id != color_id) {
            handle_color_change(id);
        }
    });

    handle_color_change(-1);

    auto* grouplayout = new QVBoxLayout();
    grouplayout->setContentsMargins(0, 0, 0, 0);
    grouplayout->setSpacing(0);
    grouplayout->addWidget(selector, 1);
    grouplayout->addWidget(sat, 0);

    setLayout(grouplayout);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}


} // namespace cosel


#endif // cosel_qt_2d_widget_h
