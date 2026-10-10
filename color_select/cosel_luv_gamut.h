// SPDX-License-Identifier: MIT
// Copyright (c) 2026 David Billen

// Utilities for accessing the gamut in CIELUV color space. Used by hsv2 class.

// TODO: This is color space dependent and instantiated statically, solidifying the static
// dependency on color spaces. It only needs to be instantiated outside of usage to avoid
// constructing it every color conversion. Find a better way.

#ifndef cosel_luv_gamut_h
#define cosel_luv_gamut_h

#include "cosel_geometry.h"
#include "cosel_triplet.h"
#include "cosel_model.h"

namespace cosel {

class luv_gamut {
public:
    using uv_t = point<double>;

    luv_gamut() noexcept;

    // Returns outer edge of gamut at L at the same angle as uv
    uv_t gamut_max(const uv_t& uv) const noexcept;

    // Convert a HUE value to a uv angle
    double hue_to_angle(double hue) const noexcept;
    // Convert a uv angle to a HUE value
    double angle_to_hue(double angle) const noexcept;

    // L value certain to contain the full gamut
    constexpr double safe_L() const {
        return safe_L_;
    }

    // Polar C value certain to be in gamut at safe_L() for any hue
    constexpr double safe_C() const {
        return safe_C_;
    }

    // Helper, return luv for angle at safe values
    luv safe_luv(double angle) const {
        return luv(safe_L_, std::cos(angle)*safe_C_, std::sin(angle)*safe_C_);
    }
private:
    static constexpr double safe_L_ = 25;
    static constexpr double safe_C_ = 70;

    // Red, green, blue
    std::array<uv_t, 3> rgb_coords;
    double red_angle = 0;

    uv_t find_intersection(const uv_t& ray, const uv_t& p1_, const uv_t& p2_) const;

    double find_red() const;
};

inline luv_gamut::luv_gamut() noexcept {
    const auto lscale = L_to_y(safe_L_);

    const auto rgb_red = rgb(1, 0, 0).forced(lscale);
    const auto rgb_green = rgb(0, 1, 0).forced(lscale);
    const auto rgb_blue = rgb(0, 0, 1).forced(lscale);

    rgb_coords = {
        luv(rgb_red).uv(),
        luv(rgb_green).uv(),
        luv(rgb_blue).uv()
    };

    red_angle = rgb_coords[0].angle();
}

inline luv_gamut::uv_t luv_gamut::gamut_max(const uv_t& uv) const noexcept {
    constexpr double epsilon = 1e-9;
    const auto d = uv.normalized();
    auto best_t = std::numeric_limits<double>::infinity();
    uv_t best;

    for(int i = 0; i < 3; ++i) {
        const uv_t& a = rgb_coords[i];
        const uv_t& b = rgb_coords[(i + 1) % 3];
        const auto num = a.cross(b);
        const auto den = d.cross(b - a);

        if(std::abs(den) < 1e-14) {
            // nearly parallel
            continue;
        }

        const auto t = num/den;

        if(t <= 0.0) {
            continue;
        }

        // Does the hit lie on the finite segment?
        const auto dx = b.x() - a.x();
        const auto dy = b.y() - a.y();
        const auto s = std::abs(dx) >= std::abs(dy)
                       ? (d.x()*t - a.x())/dx
                       : (d.y()*t - a.y())/dy;

        if(s < -epsilon || s > 1.0 + epsilon) {
            continue;
        }

        if(t < best_t) {
            best_t = t;
            best = d*t;
        }
    }

    assert(best_t < std::numeric_limits<double>::infinity());
    return best;
}

inline double luv_gamut::hue_to_angle(double hue) const noexcept {
    return abs_radian(hue*pi()*2 + red_angle);
}

inline double luv_gamut::angle_to_hue(double angle) const noexcept {
    auto h = abs_radian(angle - red_angle)/(pi()*2);
    constexpr auto eps = 1e-6;
    if (h < eps || h > 1.0 - eps)
        h = 0.0;
    return h;
}


} // namespace cosel


#endif // cosel_luv_gamut_h
