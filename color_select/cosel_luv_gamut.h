// SPDX-License-Identifier: MIT
// Copyright (c) 2026 David Billen

// Utilities for accessing the gamut in CIELUV color space.

#ifndef cosel_luv_gamut_h
#define cosel_luv_gamut_h

#include <cassert>

#include "cosel_geometry.h"
#include "cosel_space.h"
#include "cosel_transforms.h"

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

    // L value certain to contain the full gamut.
    //
    // In LUV the gamut is always represented by a triangle unless the L level exceeds the full
    // brightness of one or more RGB components - then it becomes a more complex polygon. safe_L is
    // certain to be darker than pure blue in any realistic color space, ensuring we get the
    // triangle, (but not so close to black it causes precision issues in RGB).
    constexpr double safe_L() const {
        return safe_L_;
    }

    // Polar C value used as starting point to find gamut edge
    constexpr double safe_C() const {
        return safe_C_;
    }

    // TODO: This is color space dependent and instantiated statically which adds another component
    // to the static dependency on color spaces. Find a better way.
    static const luv_gamut& instance() {
        static luv_gamut inst_;
        return inst_;
    }

private:
    static constexpr double safe_L_ = 25;
    static constexpr double safe_C_ = 30;

    // Red, green, blue
    std::array<uv_t, 3> rgb_coords;
    double red_angle = 0;

    uv_t find_intersection(const uv_t& ray, const uv_t& p1_, const uv_t& p2_) const;

    double find_red() const;
};

inline luv_gamut::luv_gamut() noexcept {
    // To keep construction options as flexible as possible, this code is not dependent on the
    // color model classes (rgb/luv).

    // Convert safe L to xyY type luminance
    const auto y = L_to_y(safe_L_);

    const auto& gamut = color_space().gamut();
    const auto& lums = gamut.luminance_mx();

    // Get RGB in LUV coordinates at safe_L
    const auto rgb_red = triplet(y/lums.comp[0], 0, 0);
    const auto xyz_red = gamut.rgb2xyz(rgb_red);
    const auto luv_red = xyz_to_luv(xyz_red, gamut.xyz_white());

    const auto rgb_green = triplet(0, y/lums.comp[1], 0);
    const auto xyz_green = gamut.rgb2xyz(rgb_green);
    const auto luv_green = xyz_to_luv(xyz_green, gamut.xyz_white());

    const auto rgb_blue = triplet(0, 0, y/lums.comp[2]);
    const auto xyz_blue = gamut.rgb2xyz(rgb_blue);
    const auto luv_blue = xyz_to_luv(xyz_blue, gamut.xyz_white());

    // Save the coordinate.
    rgb_coords = {
        uv_t{luv_red.comp[1], luv_red.comp[2]},
        uv_t{luv_green.comp[1], luv_green.comp[2]},
        uv_t{luv_blue.comp[1], luv_blue.comp[2]}
    };

    red_angle = rgb_coords[0].angle();
}

inline luv_gamut::uv_t luv_gamut::gamut_max(const uv_t& uv) const noexcept {
    constexpr double epsilon = 1e-9;
    const auto d = uv.normalized();
    auto best_t = std::numeric_limits<double>::infinity();
    uv_t best;

    for(int i = 0; i < 3; ++i) {
        const auto& a = rgb_coords[i];
        const auto& b = rgb_coords[(i + 1) % 3];
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

    if(h < eps || h > 1.0 - eps) {
        h = 0.0;
    }

    return h;
}


} // namespace cosel


#endif // cosel_luv_gamut_h
