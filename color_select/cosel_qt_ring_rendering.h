// SPDX-License-Identifier: MIT
// Copyright (c) 2026 David Billen

// Code to render hue ring. This is separated from cosel_qt_ring to declutter that source module.

#ifndef cosel_qt_ring_rendering_h
#define cosel_qt_ring_rendering_h

#include <memory>

#include <QImage>
#include <QPainter>
#include <QPixmap>
#include <QEvent>
#include <QWidget>

#include "cosel_gradient.h"
#include "cosel_model.h"
#include "cosel_qt_tools.h"

#include <cmath>
#include <algorithm>
#include <span>

namespace cosel::details {

////////////////////////////////////////////////////////////////////////////////////////////////////
// Ring class
//

class hue_ring {

public:
    hue_ring() = default;
    explicit hue_ring(double radius);

    // Radius
    double radius() const {
        return radius_;
    }

    // Inner radius
    double inner_radius() const {
        return radius_*inner_scale;
    }

    double inner_radius_scale() const {
        return inner_scale;
    }

    // Function to render the hue ring at radius size
    [[nodiscard]] QPixmap render();


    ////////////////////////////////////////////////////////////////////////////////////////////////
    // All of the following functions are for interacting with widget coordinates. The ring must be
    // perfectly centered in the widget in order to get accurate results

    // Determine if a widget coordinate is inside the ring.
    bool inside(const QPointF& pt) const;

    // Determine the angle of line from the center to an x, y coordinate. It doesn't care if the
    // coordinate is inside the ring.
    double get_angle(const QPointF& point) const;

    // Determine the angle of a specified hue
    double get_angle_from_hue(double hue) const;

    // Determines the hue at the specified point. It doesn't matter if it's inside the ring.
    double get_hue_from_point(const QPointF& point) const;

    // Determines the point of the specified hue. It is on the outer edge.
    QPointF get_point_from_hue(double hue) const;

    // Like get_point_from_hue() except the return is a normal around zero (with inverted y), in
    // our own geometry class.
    point<qreal> get_normal_angle_from_hue(double hue) const;

private:
    using hue_gradient_t = hsx_hue_gradient_t<hsv>;

    // Gradient and resulting rgbs
    std::shared_ptr<gradient_base_t> gradient;
    std::vector<rgb> rgbs;

    // Current ring_radius of the ring
    double radius_ = 0;

    // Number of pixels along the edge to use for cheap anti-aliasing.
    constexpr static double smoothing = 3;

    // The outer edge of the ring is at radius. The inner edge is this value times radius
    constexpr static double inner_scale = .75;

    // Helper to calculate gradient size based on ring radius
    static size_t radius_to_count(double radius, double scale = 2) {
        return size_t(pi()*scale*radius);
    }

    inline double calc_angle(const QPointF& point) const {
        const auto x = point.x() - radius_;
        const auto y = point.y() - radius_;
        return abs_radian(std::atan2(y, x));
    }
};


////////////////////////////////////////////////////////////////////////////////////////////////////
// Implementations

inline bool hue_ring::inside(const QPointF& pt) const {
    const auto fx = pt.x() - radius_;
    const auto fy = pt.y() - radius_;
    const auto distance_squared = square(fx) + square(fy);

    return distance_squared <= square(radius_) &&
           distance_squared >= square(radius_*inner_scale);
}

inline double hue_ring::get_angle(const QPointF& point) const {
    return calc_angle(point);
}

inline double hue_ring::get_angle_from_hue(double hue) const {
    const auto pos = gradient->source_to_gradient(hue);
    return pos*2*pi();
}

inline hue_ring::hue_ring(double radius) : radius_(radius) {
    gradient = std::make_shared<hue_gradient_t>(1, 1, radius_to_count(radius));
}

inline double hue_ring::get_hue_from_point(const QPointF& point) const {
    if(rgbs.empty()) {
        return 0;
    }

    const auto angle = calc_angle(point);

    const auto pos = angle/(2*pi());
    return gradient->gradient_to_source(pos);
}

inline point<qreal> hue_ring::get_normal_angle_from_hue(double hue) const {
    const auto angle = get_angle_from_hue(hue);
    return point<qreal>(std::cos(angle), std::sin(angle));
}

inline QPointF hue_ring::get_point_from_hue(double hue) const {
    const auto normal = get_normal_angle_from_hue(hue);
    const auto center = point<qreal>(radius_, radius_);

    const auto t = center + normal*radius_;
    return QPointF(t.x(), t.y());
}

inline QPixmap hue_ring::render() {
    if(radius_ < 1) {
        return QPixmap();
    }

    rgbs.reserve(gradient->size() + 4);
    gradient->get_rgbs(rgbs);
    rgbs.emplace_back(rgbs[0]); // oversize to avoid a range check inner loop

    const auto count = int(rgbs.size() - 1);

    // Setup for writing QImage

    QImage image(radius_*2, radius_*2, QImage::Format_RGBA8888);
    image.fill(qRgba(0, 0, 0, 0));

    const int word_pitch = image.bytesPerLine() / 4;
    auto* const pixels = reinterpret_cast<uint32_t*>(image.bits());

    // Setup constants for rendering ring

    const double radiusf = radius_;
    const int radius2 = radius_*2;

    const auto outer = radiusf;
    const auto inner = outer*inner_scale;
    const auto outer_squared = square(outer);
    const auto inner_squared = square(inner);

    const auto scale = count/(2*pi());

    // Setup smoothing constants

    constexpr double smoothing = 3;
    constexpr auto smooth_alpha_scale = 255/(smoothing + 1);
    const auto outer_smooth_range = outer_squared - square(outer - smoothing);
    const auto inner_smooth_range = square(inner + smoothing) - inner_squared;

    const rgb* const NOALIAS colors = rgbs.data();

    // Render ring

    for(int y = 0; y < radius2; ++y) {
        const auto fy = y - radiusf;
        auto* const NOALIAS row = &pixels[(radius2 - y - 1) * word_pitch];

        for(int x = 0; x < radius2; ++x) {
            const auto fx = x - radiusf;
            const auto dist_squared = fx*fx + fy*fy;

            if(dist_squared >= inner_squared && dist_squared <= outer_squared) {

                // x, y coordinate is inside the ring...

                // Find the color. The gradient is expected to be large enough that interpolation would
                // not make a perceptible difference.
                const auto angle = abs_radian(std::atan2(fy, fx));
                const auto pos = angle*scale;
                const auto index = size_t(pos + .5);
                const auto color = colors[index];

                // Calculate "smoothing" alpha to anti-alias the borders

                int alpha = 255;

                if(outer_squared - dist_squared < outer_smooth_range) {
                    const auto dist = sqrt(dist_squared);
                    alpha = int(square(outer - dist + 1)*smooth_alpha_scale);
                    alpha = std::min(alpha, 255);
                } else if(dist_squared - inner_squared < inner_smooth_range) {
                    const auto dist = sqrt(dist_squared);
                    alpha = int(square(dist - inner + 1)*smooth_alpha_scale);
                    alpha = std::min(alpha, 255);
                }

                // Convert the color and write to image pixel
                row[x] = to_image_pixel(color, alpha);
            }
        }
    }

    return QPixmap::fromImage(std::move(image));
}

} // namespace cosel::details



#endif // cosel_qt_ring_rendering_h
