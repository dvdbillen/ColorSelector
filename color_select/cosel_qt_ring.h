// SPDX-License-Identifier: MIT
// Copyright (c) 2026 David Billen

// Implement HSV selection "ring" widget

#ifndef cosel_qt_ring_h
#define cosel_qt_ring_h

#include <algorithm>
#include <cmath>
#include <memory>
#include <semaphore>
#include <span>
#include <thread>

#include <QDebug>
#include <QImage>
#include <QPainter>
#include <QPixmap>
#include <QEvent>
#include <QTimer>
#include <QWidget>

#include "cosel_gradient.h"
#include "cosel_model.h"
#include "cosel_model_hsx.h"
#include "cosel_qt_ring_rendering.h"
#include "cosel_qt_ringtri_rendering.h"
#include "cosel_qt_throttled.h"
#include "cosel_qt_tools.h"
#include "cosel_util.h"

namespace cosel {

class ring_selector : public QWidget {
public:
    explicit ring_selector(const selected_color_t& selection, QWidget* parent_);

    ~ring_selector() = default;
    ring_selector(const ring_selector&) = delete;
    ring_selector& operator =(const ring_selector&) = delete;
    ring_selector(ring_selector&&) = delete;
    ring_selector& operator =(ring_selector&&) = delete;

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent([[maybe_unused]] QMouseEvent* event) override;
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

    void showEvent([[maybe_unused]] QShowEvent* event) override {
        set_color(get_color(), false, true);
        // Start the throttled rendering timer and worker
        triangle.start();
    }
    void hideEvent([[maybe_unused]] QHideEvent* event) override {
        // Stop throttled rendering timer and worker while hidden to be polite
        triangle.stop();
    }

    QSize sizeHint() const override {
        if(ring_pixmap.isNull()) {
            return QSize(32, 32);
        }

        return ring_pixmap.deviceIndependentSize().toSize();
    }
private:
    using gpoint_t = point<qreal>;

    enum class input_state_t {
        // Pointer not down
        released,
        // Pointer down and was in ring when it got that way
        ring_pressed,
        // Pointer down and was in triangle when it got that way
        triangle_pressed
    };

    input_state_t input_state = input_state_t::released;

    selected_color_t selection_;
    unsigned selection_id = 0;

    // Used for determining position when hue is invalid (color is gray)
    double last_valid_hue = 0;

    hsv valid_color(const hsv& c) const {
        if(!hsv::is_gray(c.s(), c.v())) {
            return c;
        }

        return c.with_h(last_valid_hue);
    }

    void set_color(const hsv& color_, bool xmit, bool rethink_triangle) {
        auto color = valid_color(color_);
        last_valid_hue = color.h();

        if(rethink_triangle) {
            // An update() will be issued when the triangle finishes concurrently rendering.
            update_triangle(ring.get_angle_from_hue(color.h()), color);
        } else {
            // rethink_triangle is only false when interactively adjusting the triangle in which
            // case the triangle itself did not change in position or color and the pixmap does not
            // need to be regenerated.
            update();
        }

        if(xmit) {
            selection_->set_rgb(color.to_rgb(), selection_id);
        }
    }

    hsv get_color() const {
        const auto rgb_c = selection_->rgb();
        auto hsv_c = hsv(rgb_c);
        return valid_color(hsv_c);
    }

    void set_hue(double hue) {
        last_valid_hue = hue;
        const auto c = get_color().with_h(last_valid_hue);
        set_color(c, true, true);
    }

    //////////////////////////////////////////////////////////
    // 1:1 pixmap helpers
    //
    // These help the UI cope with the pixmaps that are rendered centered with a 1:1 pixel ratio
    // regardless of Qt's graphics scaling and Y increasing upward

    // Convert a widget coordinate into a pixmap coordinate
    [[nodiscard]] QPointF widget_to_pixmap(const QPixmap& pm, const QPointF& pt) const {
        const auto center = gpoint_t(.5*width(), .5*height());
        const auto scaled_pt = (geo(pt) - center)*devicePixelRatioF();

        const auto pm_center = gpoint_t(.5*pm.width(), .5*pm.height());
        return geo(pm_center + scaled_pt.flipped_y());
    }

    // Convert a pixmap coordinate into a widget coordinate
    [[nodiscard]] QPointF pixmap_to_widget(const QPixmap& pm, const QPointF& pt) const {
        const auto pm_center = gpoint_t(.5*pm.width(), .5*pm.height());
        const auto scaled_pt = (geo(pt) - pm_center)*(1/devicePixelRatioF());

        const auto center = gpoint_t(.5*width(), .5*height());
        return geo(center + scaled_pt.flipped_y());
    }

    // Find centered position to render pixmap
    [[nodiscard]] QPointF center_pixmap_offset(const QPixmap& pm) const {
        const auto dpr = pm.devicePixelRatio();
        const auto pixmap_render_size = QSizeF(pm.width()/dpr, pm.height()/dpr);
        const auto x = (width() - pixmap_render_size.width())*.5;
        const auto y = (height() - pixmap_render_size.height())*.5;
        return QPointF(x, y);
    }

    void rebuild();

    // Ring

    details::hue_ring ring;
    // Pixmap for ring
    QPixmap ring_pixmap;


    // Triangle

    throttled_image<details::hue_triangle> triangle;

    void update_triangle(float radius, float rotation, hsv color) {
        triangle.update(details::hue_triangle(radius, rotation, color));
    }

    void update_triangle(float rotation, hsv color) {
        update_triangle(triangle.info().get_radius(), rotation, color);
    }
};


////////////////////////////////////////////////////////////////////////////////////////////////////
// Triangle implementations (except rendering)

inline ring_selector::ring_selector(const selected_color_t& selection, QWidget* parent_) :
    QWidget(parent_),
    selection_(selection),
    triangle(this, true) {
    setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);

    selection_id = selection_->assign_id();

    connect(selection_.get(), &selected_color::changed, this, [this](unsigned id) {
        if(isVisible() && id != selection_id) {
            set_color(get_color(), false, true);
        }
    });
}

inline void ring_selector::mousePressEvent(QMouseEvent* event) {
    if(ring_pixmap.isNull() || triangle.pixmap().isNull()) {
        return;
    }

    const auto posf = real(event->pos());

    auto pos = widget_to_pixmap(ring_pixmap, posf);
    const auto ring_intersect = ring.inside(pos);

    if(ring_intersect) {
        // Mouse press intended to adjust the ring hue position, not the position in the triangle
        set_hue(ring.get_hue_from_point(pos));
        input_state = input_state_t::ring_pressed;
        update();
        return;
    }

    pos = widget_to_pixmap(triangle.pixmap(), posf);

    const auto tri_intersect = triangle.info().inside_radius(pos);

    if(!tri_intersect) {
        // No intersection - ignore the press
        input_state = input_state_t::released;
        return;
    }

    // Mouse press intended to adjust the sat/level position in the triangle

    pos = triangle.info().clip_ray_to_triangle_bounds(pos);

    const auto sv = triangle.info().get_sv_from_point(pos);
    set_color(get_color().with_s(sv.first).with_v(sv.second), true, false);
    input_state = input_state_t::triangle_pressed;
}

inline void ring_selector::mouseMoveEvent(QMouseEvent* event) {
    if(input_state == input_state_t::ring_pressed) {

        // We are adjusting the ring position (hue)
        auto pos = widget_to_pixmap(ring_pixmap, real(event->pos()));
        const auto hue = ring.get_hue_from_point(pos);
        set_hue(hue);
        update();
        return;
    }

    if(input_state == input_state_t::triangle_pressed) {

        // We are adjusting the position in the triangle (sat/level)
        auto pos = widget_to_pixmap(triangle.pixmap(), real(event->pos()));
        pos = triangle.info().clip_ray_to_triangle_bounds(pos);
        const auto sv = triangle.info().get_sv_from_point(pos);
        set_color(get_color().with_s(sv.first).with_v(sv.second), true, false);
    }
}

inline void ring_selector::mouseReleaseEvent([[maybe_unused]] QMouseEvent* event) {
    input_state = input_state_t::released;
}

inline void ring_selector::paintEvent([[maybe_unused]] QPaintEvent* event) {

    const auto& triangle_pixmap = triangle.pixmap();

    if(ring_pixmap.isNull() || triangle_pixmap.isNull()) {
        return;
    }

    // Ring and triangle are both built from the widget's current ratio. A monitor change can
    // update that ratio without another resize, which leaves the ring wrong until something
    // renders it again.
    if(!qFuzzyCompare(ring_pixmap.devicePixelRatio(), devicePixelRatioF())) {
        rebuild(); // This will trigger yet another paint event
    }

    QPainter p(this);
    const auto color = get_color();

    // Render ring
    const auto ring_offset = center_pixmap_offset(ring_pixmap);
    p.drawPixmap(ring_offset.x(), ring_offset.y(), ring_pixmap);

    // Render ring position marker (using triangle rotation)
    const auto angle = ring.get_angle_from_hue(color.h());
    const auto normal = gpoint_t(std::cos(angle), -std::sin(angle)); // note flipped Y
    const auto center = gpoint_t(.5*width(), .5*height());
    const auto radius = ring.radius()/ring_pixmap.devicePixelRatio();
    const auto p1 = center + normal*radius*ring.inner_radius_scale();
    const auto p2 = center + normal*radius;

    auto pen = QPen(QColor(0, 0, 0));
    pen.setWidth(2);
    p.setPen(pen);
    p.setRenderHint(QPainter::Antialiasing);
    p.drawLine(geo(p1), geo(p2));


    // Render triangle
    const auto tri_offset = center_pixmap_offset(triangle_pixmap);
    p.drawPixmap(tri_offset.x(), tri_offset.y(), triangle_pixmap);

    // Render triangle position marker
    constexpr qreal uv_mark_r = 3;
    const auto uv_pos = triangle.info().get_point_from_sv(color.s(), color.v());
    const auto pos = pixmap_to_widget(triangle_pixmap, uv_pos);
    const auto uv_rect = QRectF(pos - QPointF(uv_mark_r, uv_mark_r),
                                QSizeF(uv_mark_r*2 + 1, uv_mark_r*2 + 1));

    pen.setColor(QColor(255, 255, 255));
    pen.setWidth(1);
    p.setPen(pen);
    p.drawEllipse(uv_rect);

    pen.setColor(QColor(0, 0, 0));
    p.setPen(pen);
    p.drawEllipse(uv_rect.adjusted(-1, -1, 2, 2));
}

inline void ring_selector::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    rebuild();
}

inline void ring_selector::rebuild() {
    const auto dpr = devicePixelRatioF();
    const double w = width();
    const double h = height();
    const auto m = std::min(w, h);
    const auto radius = (m - 4)/2*dpr;
    ring = details::hue_ring(radius);
    const auto color = get_color();
    const auto angle = ring.get_angle_from_hue(color.h());
    // 4 is a logical gap. inner_radius() is already in device pixels.
    const auto triradius = ring.inner_radius() - 4*dpr;
    update_triangle(triradius, angle, color);
    ring_pixmap = ring.render();
    ring_pixmap.setDevicePixelRatio(dpr);
    update();
}



} // namespace cosel


#include "cosel_qt_ring_rendering.h"

#endif // cosel_qt_ring_h

