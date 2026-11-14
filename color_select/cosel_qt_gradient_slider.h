// SPDX-License-Identifier: MIT
// Copyright (c) 2026 David Billen

/// @file cosel_qt_gradient_slider.h
/// @brief Gradient legend custom slider implementation

#ifndef cosel_qt_gradient_slider_h
#define cosel_qt_gradient_slider_h

#include <array>

#include <QBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <QTimer>
#include <QWidget>

#include "cosel_geometry.h"
#include "cosel_gradient.h"
#include "cosel_qt_custom_slider.h"
#include "cosel_qt_gradpixmap.h"
#include "cosel_qt_render_helper.h"
#include "cosel_qt_slider_label.h"
#include "cosel_qt_swatch.h"

namespace cosel {

constexpr unsigned gradient_slider_knob_width = 24;
constexpr unsigned seg_size = 1;
constexpr unsigned seg_gap = 0;

/// @brief Slider that displays a gradient legend using a custom_slider.
///
/// @details This class is normally used as part of a set of sliders and has doxygen documentation
/// only to define the @ref gradient_slider::style_options_t.
class gradient_slider : public QWidget {
    Q_OBJECT;

public:
    /// @brief Struct that contains options for the slider
    struct style_options_t {
        /// Height of slider in device pixels. Can't be less than gradient_slider_knob_width.
        int height = konst::default_slider_height();
        /// Set true to make pointer (knob) on top pointing down. Default is pointing up.
        bool pointer_top = false;
        /// Corner rounding 0 to 1. Matches Qt rounding "absolute size" calculated as:
        /// rounding*min(width,height)/2
        double rounding = konst::default_corner_rounding();
        /// Show label and value.
        ///
        /// @details Set this false if you want to repurpose this class to just create a gradient
        /// slider that is not in a layout with a label on the left and an editable display value on
        /// the right.
        bool info = true;
    };

    // slider_label is normally one character, e.g. "R", "G", "B".
    // display_min is low range for the display scale
    // isplay_max is high range for the display scale
    // options_ is an instance of gradient_slider::style_options_t
    // current_ is the initial position of the slider (using the display range not a normal)
    explicit gradient_slider(QString slider_label, double display_min, double display_max,
                             style_options_t options_, double current_, QWidget* parent = nullptr);

    // Associatea a gradient_base_t derived gradient with the slider. All gradient sliders must
    // have a gradient associated with them.
    void set_gradient(const std::shared_ptr<gradient_base_t>& grad);

    const std::shared_ptr<gradient_base_t>& gradient() const {
        return gradient_;
    }

    void set_value(double value) {
        set_value(value, true);
    }
    double value() const {
        return current;
    }

    // This calculates a new value at the same relative position. It does not emit a signal.
    void change_display_scale(double display_min, double display_max) {
        display_min_ = display_min;
        display_max_ = display_max;

        // TODO: Work out if it would be better to use value_to_normal(current) at the top
        set_value(slider->value(), false);
    }
private:
    QColor bg_color;
    QColor fg_color;
    QString label_;
    double display_min_ = 0;
    double display_max_ = 100;
    style_options_t options;
    QPixmap pointer_pixmap;
    double pointer_dpr = 0;


    constexpr static int minimum_width = 64;

    // Shared pointer to gradient
    std::shared_ptr<gradient_base_t> gradient_;
    // Current value of slider
    double current = 0;
    // See the custom slider class. That class is designed to be used as inherited or compositional.
    // Here we choose the latter which requires a lambda to be assigned for the rendering.
    custom_slider* slider = nullptr;
    // The descriptive, (typically single letter), label positioned to the left of the slider
    slider_label_display* label = nullptr;
    // The editable display of the slider value position to the right of the slider
    slider_label_edit* edit = nullptr;

    // Color vector returned by the gradient
    std::vector<rgb> gradient_colors;
    // Gradient is rendered to this pixmap
    QPixmap gradient_pixmap;

    // Convert a normal from the custom_slider into a scaled value
    double normal_to_value(double n) const {
        return lerp(display_min_, display_max_, n);
    }

    // Convert a scaled value into a normal as used by the custom_slider
    double value_to_normal(double v) const {
        return unlerp(display_min_, display_max_, v);
    }

    // The gradients may not be linear because the colors are redistributed for perceptual
    // uniformity. Since the slider is pointing at a color in the gradient, this function is needed
    // to convert the corresponding (non-linear) gradient position into a useable linear position
    // (and scale it).
    double slider_to_value(double n) const {
        assert(gradient_);
        const auto source_pos = gradient_->gradient_to_source(n);
        const auto v = normal_to_value(source_pos);
        return v;
    }

    // This does the reverse of slider_to_value, (see the comment on that function). It converts a
    // linear position into a normal and returns the corresponding non-linear gradient position.
    double value_to_slider(double v) const {
        assert(gradient_);
        const auto n = value_to_normal(v);
        const auto gradient_pos = gradient_->source_to_gradient(n);
        return gradient_pos;
    }

    // This returns the pixmap size which is not straightforward because it excludes the size of the
    // pointer (knob) which can overhang the edges, and also considers that we render the pixmaps
    // with 1:1 pixel resolution regardless of display scaling.
    QSize calc_pixmap_size() const {
        assert(slider != nullptr);
        const auto slider_width = slider->width();
        const auto knob_width = slider->knob_size().width();
        const auto w = std::max(slider_width - knob_width, knob_width + 4);
        const auto h = slider->knob_size().height();
        const auto dpr = devicePixelRatioF();
        const auto pixel_size = QSizeF(w*dpr, h*dpr);
        return round(pixel_size);
    }

    // Function to render the contents of the custom_slider.
    void slider_painter(QPainter* p, const QRect* r, const QRect* kr, slider_state state);

    // Private version of set_value that provides option to sync the slider.
    void set_value(double value, bool sync) {
        current = std::clamp(value, display_min_, display_max_);

        if(edit != nullptr) {
            edit->set(current);
        }

        if(sync) {
            const auto n = value_to_slider(current);
            slider->set(n);
        }
    }

    void resizeEvent([[maybe_unused]] QResizeEvent* ev) override;
signals:
    void changed();
};


////////////////////////////////////////////////////////////////////////////////////////////////////
inline gradient_slider::gradient_slider(QString slider_label, double display_min,
                                        double display_max,
                                        style_options_t options_, double current_, QWidget* parent) :
    QWidget(parent),
    label_(std::move(slider_label)),
    display_min_(display_min),
    display_max_(display_max),
    options(options_),
    current(current_) {

    std::tie(bg_color, fg_color) = get_bg_fg_colors(this);

    slider = new custom_slider(0, options.height, gradient_slider_knob_width);

    // The incrementer is used by the keyboard control to decide how much distance to travel for
    // a right/left arrow press.
    slider->set_incrementer([this](double n, bool dec) {
        auto v = std::floor(normal_to_value(n));
        v += dec? -2: 2;
        v = std::clamp(v, this->display_min_, this->display_max_);
        return value_to_normal(v);
    });

    // Set the lambda for rendering the custom slider to our slider_paint() function.
    slider->set_painter([this](QPainter* p, const QRect* r, const QRect* kr, slider_state state) {
        slider_painter(p, r, kr, state);
    });

    // Handle custom slider changes
    connect(slider, &custom_slider::changed, this, [this]() {
        const auto n = slider->value();
        const auto value = slider_to_value(n);
        set_value(value, false);
        emit changed();
    });

    // Create label and editbox if the options allow it
    if(options_.info) {
        label = new slider_label_display(label_);
        label->setFixedSize(gradient_slider_knob_width, gradient_slider_knob_width);

        // TODO: Maybe calculate precision to handle the possibility of 0-1 range with more than 2.
        edit = new slider_label_edit(display_min, display_max, 2);
        edit->setMinimumSize(gradient_slider_knob_width*2, options.height);
        edit->setMaximumSize(gradient_slider_knob_width*3, options.height);

        connect(edit, &slider_label_edit::edited, this, [this](double n) {
            set_value(n);
            emit changed();
        });

        label->setFocusPolicy(Qt::NoFocus);
        edit->setFocusPolicy(Qt::StrongFocus);
    }

    slider->setFocusPolicy(Qt::StrongFocus);

    // Handle the layout

    auto* layout = new QHBoxLayout();
    layout->setContentsMargins(0, 0, 0, 0);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    setMinimumWidth(minimum_width);
    setMinimumHeight(std::max<int>(options.height, gradient_slider_knob_width));

    if(label != nullptr) {
        layout->addWidget(label);
    }

    layout->addWidget(slider, 100);

    if(edit != nullptr) {
        layout->addWidget(edit);
    }

    setLayout(layout);

    // Defer setting the slider to the value
    // defer(25, [this]() {
    //     slider->setFocus();
    //     set_value(current);
    //     emit changed();
    // });
}


////////////////////////////////////////////////////////////////////////////////////////////////////
inline void gradient_slider::set_gradient(const std::shared_ptr<gradient_base_t>& grad) {
    gradient_ = grad;

    if(slider == nullptr) {
        return;
    }

    // Resync the knob position to the (potentially non-linear) gradient which may have changed
    const auto n = value_to_slider(current);
    slider->set(n);

    // Regenerate the pixelmap and update
    const auto pixmap_size = calc_pixmap_size();

    const auto gradient_size = pixmap_size.width()/seg_size;

    if(gradient_->size() != unsigned(gradient_size)) {
        gradient_->resize(size_t(gradient_size));
    }

    gradient_->get_rgbs(gradient_colors);

    if(seg_size == 1) {
        gradient_pixmap = QPixmap::fromImage(
                              make_gradient_image(gradient_colors, pixmap_size.height(), options.rounding));
    } else {
        gradient_pixmap = QPixmap::fromImage(
                              make_gradient_image(
                                  gradient_colors, pixmap_size.height(), QColor(0, 0, 0), seg_size,
                                  pixmap_size.height(), 0, seg_gap));
    }

    gradient_pixmap.setDevicePixelRatio(devicePixelRatioF());
    slider->update();
}


////////////////////////////////////////////////////////////////////////////////////////////////////
inline void gradient_slider::resizeEvent([[maybe_unused]] QResizeEvent* ev) {
    if(slider == nullptr) {
        return;
    }

    const auto pixmap_size = calc_pixmap_size();

    if(pixmap_size.width() < 4 || pixmap_size.height() < 4) {
        return;
    }

    const auto gradient_size = pixmap_size.width()/seg_size;

    // Regenerate the gradient to the new size and update
    gradient_->resize(size_t(gradient_size));
    gradient_->get_rgbs(gradient_colors);

    if(seg_size == 1) {
        gradient_pixmap = QPixmap::fromImage(
                              make_gradient_image(gradient_colors, pixmap_size.height(), options.rounding));
    } else {
        gradient_pixmap = QPixmap::fromImage(
                              make_gradient_image(
                                  gradient_colors, pixmap_size.height(), QColor(0, 0, 0), seg_size,
                                  pixmap_size.height(), 0, seg_gap));
    }

    gradient_pixmap.setDevicePixelRatio(devicePixelRatioF());
    slider->update();
}


////////////////////////////////////////////////////////////////////////////////////////////////////
inline void gradient_slider::slider_painter(QPainter* p, [[maybe_unused]] const QRect* r,
        const QRect* kr, slider_state state) {

    if(gradient_pixmap.isNull()) {
        return;
    }

    // Render gradient legend
    const auto dpr = devicePixelRatioF();
    const auto sizef = real(gradient_pixmap.size());
    const auto unscaled = QSizeF(sizef.width()/dpr, sizef.height()/dpr);
    QRect real_r(QPoint(slider->knob_size().width()/2, 0), round(unscaled));

    p->drawPixmap(real_r.x(), real_r.y(), gradient_pixmap);

    // Outline it
    const auto outline_rect = QRectF(real_r).adjusted(.5, .5, -.5, -.5);

    p->setRenderHint(QPainter::Antialiasing);
    QPen outline_pen(with_alpha(fg_color, 48));
    outline_pen.setCosmetic(true);
    p->setPen(outline_pen);
    p->setBrush(Qt::NoBrush);

    if(options.rounding > 0) {
        // Round to integer to match rounding in generated legend
        const auto round_linesf = options.rounding*(gradient_pixmap.height());
        const auto round_lines = round_linesf/devicePixelRatioF();
        const auto radius = round_lines/2;
        p->drawRoundedRect(outline_rect, radius, radius);
    } else {
        p->drawRect(outline_rect);
    }

    // Render pointer if enabled
    if(state == slider_state::disabled) {
        return;
    }

    if(pointer_pixmap.isNull() || devicePixelRatioF() != pointer_dpr) {
        pointer_pixmap = render_pointer(this, kr, options.pointer_top);
        pointer_dpr = pointer_pixmap.devicePixelRatioF();
    }

    const auto x = kr->x();
    const auto y = options.pointer_top? int(0): int((real_r.height() - kr->width()*.5) + .5);
    p->drawPixmap(x, y, pointer_pixmap);
}



} // namespace cosel


#endif // cosel_qt_gradient_slider_h
