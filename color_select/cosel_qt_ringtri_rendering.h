// SPDX-License-Identifier: MIT
// Copyright (c) 2026 David Billen

// Handles triangle for HSV ring rendering

#ifndef cosel_qt_ringtri_rendering_h
#define cosel_qt_ringtri_rendering_h

#include <memory>

#include <QEvent>
#include <QImage>
#include <QPainter>
#include <QPixmap>
#include <QPoint>
#include <QWidget>

#include "cosel_gradient.h"
#include "cosel_model.h"
#include "cosel_qt_tools.h"

#include <cmath>
#include <algorithm>
#include <span>

namespace cosel {

namespace details {

////////////////////////////////////////////////////////////////////////////////////////////////////
// Triangle class
//

class hue_triangle {
public:
    using point_t = point<float>;
    hue_triangle() = default;

    explicit hue_triangle(float radius_, float rotation_, hsv color_) :
        center(radius_, radius_),
        radius(radius_),
        rotation(rotation_),
        color(color_) {

        initialize_verts();
    }

    float get_rotation() const {
        return rotation;
    }

    float get_radius() const {
        return radius;
    }

    // Triangle rendering code
    void render_image(QImage& image) const noexcept;

    // All of the following functions are for interacting with widget coordinate. The triangle must
    // be perfectly centered in the widget in order to get accurate results

    // Ensures that a point is within the triangle bounds so it can be used with get_sv_from_point().
    QPointF clip_ray_to_triangle_bounds(const QPointF& point_) const noexcept;

    bool inside(const QPointF& point) const noexcept {
        const auto clipped = clip_ray_to_triangle_bounds(point);
        return clipped == point;
    }

    // Ensures that a point is within the triangle radius
    bool inside_radius(const QPointF& point) const noexcept {
        return (geo(point) - qreal(radius)).distance_squared() <= square(radius);
    }

    // Get SV of HSV from a specified point. It is assumed this point has been verified inside the
    // triangle(!)
    std::pair<double, double> get_sv_from_point(const QPointF& point) const noexcept;

    // Get coordinate from SV of HSV.
    QPointF get_point_from_sv(double s, double v) const noexcept;

private:
    using triangle_t = std::array<point_t, 3>;

    constexpr static float pos_2 = pif()*2/3;
    constexpr static float pos_3 = pif()*4/3;
    static const point_t rotation_2;
    static const point_t rotation_3;

    // Vertex for the triangle render shading
    struct vertex2d {
        point_t pos;
        hsv color;

        vertex2d() = default;
        explicit vertex2d(point_t pos_, hsv color_) :
            pos(pos_),
            color(color_) {}
    };

    using vert3_t = std::array<vertex2d, 3>;

    // State data
    point_t center;
    float radius = 0;
    float rotation = 0;
    hsv color;

    // Precalculated data
    vert3_t verts;
    float area = 0;
    float inv_area = 0;

    void initialize_verts();


};


////////////////////////////////////////////////////////////////////////////////////////////////////
// Implementations

inline const hue_triangle::point_t hue_triangle::rotation_2 = hue_triangle::point_t(std::cos(pos_2),
        std::sin(pos_2));
inline const hue_triangle::point_t hue_triangle::rotation_3 = hue_triangle::point_t(std::cos(pos_3),
        std::sin(pos_3));

inline void hue_triangle::initialize_verts() {
    const point_t rotation_pt(std::cos(rotation), std::sin(rotation));
    const auto h = color.h();

    verts = {
        vertex2d(point_t(center + rotation_pt*radius), hsv(h, 1, 1)),
        vertex2d(point_t(center + rotation_2.cmulled(rotation_pt)* radius), hsv(h, 0, 1)),
        vertex2d(point_t(center + rotation_3.cmulled(rotation_pt)* radius), hsv(h, 1, 0))
    };

    const auto p0 = verts[0].pos;
    const auto p1 = verts[1].pos;
    const auto p2 = verts[2].pos;

    area = (p1.x() - p0.x())*(p2.y() - p0.y()) - (p2.x() - p0.x())*(p1.y() - p0.y());
    inv_area = 1/area;
}



inline void hue_triangle::render_image(QImage& image) const noexcept {
    // setup image info
    const auto width = int(radius*2 + .5f);
    const auto height = int(radius*2 + .5f);

    // Create image if empty or the size has changed
    if(image.isNull() || image.width() != width || image.height() != height) {
        image = QImage(width, height, QImage::Format_RGBA8888);
    }

    // Clear image and setup data to access pixels
    image.fill(qRgba(0, 0, 0, 0));
    const auto word_pitch = image.bytesPerLine() / 4;
    auto* const pixels = reinterpret_cast<uint32_t*>(image.bits());


    // Transform to screen space (assume vertices already in pixel coordinates)
    // Input positions are QImage pixel coordinates

    const auto p0 = verts[0].pos;
    const auto p1 = verts[1].pos;
    const auto p2 = verts[2].pos;

    // These are for handling edge smoothing
    constexpr auto edge_fade   = 2.5f;
    constexpr auto edge_threshold  = 0.015f;
    constexpr auto edge_smooth_scale = 1.0f/(edge_fade*0.333f);


    for(int y = 0; y < height; ++y) {
        auto* const NOALIAS scanline = &pixels[(height - y - 1)*word_pitch];

        for(int x = 0; x < width; ++x) {
            const auto px = x + 0.5f;
            const auto py = y + 0.5f;

            // Barycentric coordinates via edge equations (GPU-style)
            const auto w0 = ((p1.x() - px)*(p2.y() - py) - (p2.x() - px)*(p1.y() - py))*inv_area;
            const auto w1 = ((p2.x() - px)*(p0.y() - py) - (p0.x() - px)*(p2.y() - py))*inv_area;
            const auto w2 = ((p0.x() - px)*(p1.y() - py) - (p1.x() - px)*(p0.y() - py))*inv_area;

            if(w0 >= 0.0f && w1 >= 0.0f && w2 >= 0.0f) {
                // Inside triangle
                uint8_t a = 255;

                const auto min_w = std::min({w0, w1, w2});

                if(min_w < edge_threshold) {
                    // Handle edge fade
                    const auto min_w = std::min({w0, w1, w2});
                    const auto normalized_dist = min_w/edge_threshold;
                    auto alpha = clamp(normalized_dist*edge_smooth_scale);
                    alpha = alpha*alpha*(3.0f - 2*alpha);
                    a = uint8_t(255.0f*alpha + .5f);
                }

                const auto v = w0 + w1;
                const auto s = (v > 0.0001f)? (w0/v) : 0.0f;

                hsv hsv_color(color.h(), s, v);
                const auto rgb_color = hsv_color.to_rgb();

                // Convert the color and write to image pixel
                scanline[x] = to_image_pixel(rgb_color, a);
            }
        }
    }
}

inline QPointF hue_triangle::clip_ray_to_triangle_bounds(const QPointF& point_) const noexcept {
    point_t point(point_.x(), point_.y());

    if(point.compare(center, 1.0f)) {
        return point_;
    }

    // Direction vector from center to target
    const point_t dir = point - center;

    // Test intersection against each of the 3 edges and determine closest
    auto best = 1.0f;
    auto found = false;

    // Local function to detect intersection
    auto edge_intersect = [&](const point_t& a, const point_t& b) {
        // Line from center to target: center + t*dir
        // Edge: a + s*(b - a), s in [0,1]

        const point_t edge_dir = b - a;
        const point_t to_a = a - center;

        // Denominator: dir × edge_dir (2D cross product)
        const float denom = dir.cross(edge_dir);

        if(std::abs(denom) < 1e-8f) {
            // parallel (or near-parallel) — skip or handle as no intersect
            return;
        }

        const float t = to_a.cross(edge_dir)/denom;
        const float s = to_a.cross(dir)/denom;

        if(t >= 0 && s >= 0 && s <= 1) {
            if(t < best) {
                best = t;
                found = true;
            }
        }
    };

    edge_intersect(verts[0].pos, verts[1].pos);
    edge_intersect(verts[1].pos, verts[2].pos);
    edge_intersect(verts[2].pos, verts[0].pos);

    if(!found) {
        return point_;
    }

    const auto ret = center + dir * best;
    return QPointF(ret.x(), ret.y());
}

inline QPointF hue_triangle::get_point_from_sv(double s, double v) const noexcept {
    s = clamp(s);
    v = clamp(v);

    // Inverse: w0 = s*v, w1 = v*(1 - s), w2 = 1 - v
    const float w0 = s*v;
    const float w1 = v*(1 - s);
    const float w2 = 1 - v;

    const auto fx = w0*verts[0].pos.x() + w1*verts[1].pos.x() + w2*verts[2].pos.x();
    const auto fy = w0*verts[0].pos.y() + w1*verts[1].pos.y() + w2*verts[2].pos.y();

    return QPointF(fx, fy);
}

inline std::pair<double, double> cosel::details::hue_triangle::get_sv_from_point(
    const QPointF& point) const noexcept {

    const auto p0 = verts[0].pos;
    const auto p1 = verts[1].pos;
    const auto p2 = verts[2].pos;

    const float px = point.x();
    const float py = point.y();

    // Barycentric coordinates (w0 for v0/pure, w1 for v1/white, w2 for v2/black)
    const auto w0 = ((p1.x() - px)*(p2.y() - py) - (p2.x() - px)*(p1.y() - py))*inv_area;
    const auto w1 = ((p2.x() - px)*(p0.y() - py) - (p0.x() - px)*(p2.y() - py))*inv_area;
    const auto w2 = ((p0.x() - px)*(p1.y() - py) - (p1.x() - px)*(p0.y() - py))*inv_area;

    // Clamp tiny negatives due to float precision (since assumed inside)
    const auto clamped_w0 = std::max(0.0f, w0);
    const auto clamped_w1 = std::max(0.0f, w1);
    const auto clamped_w2 = std::max(0.0f, w2);
    const auto sum = clamped_w0 + clamped_w1 + clamped_w2;
    const auto norm_w0 = (sum > 0) ? clamped_w0/sum : 0;
    const auto norm_w1 = (sum > 0) ? clamped_w1/sum : 0;

    // Compute S and V exactly as in rendering
    const auto v = norm_w0 + norm_w1;
    const auto s = (v > 0.0001f) ? (norm_w0/v) : 0;

    return {double(s), double(v)};
}


} // namespace details

} // namespace cosel



#endif // cosel_qt_ringtri_rendering_h
