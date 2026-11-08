// SPDX-License-Identifier: MIT
// Copyright (c) 2026 David Billen

// Color space support

// TODO: Implement runtime selectable color spaces if there is any interest in the feature.

#ifndef cosel_color_space_h
#define cosel_color_space_h


#include <array>

#include "cosel_triplet.h"


namespace cosel {

////////////////////////////////////////////////////////////////////////////////////////////////////
// This gamut class is not currently used at run time. It was written for the code that generated the
// predefined gamut classes which are constants. It is retained in the source to fully document the
// gamut type, both literally (with comments on the functions), and practically (the code can
// be examined to verify anything uncertain).
//
// See any of the defined color spaces for an example of the color_space type. They contain a
// function to return an instance of a gamut() and two functions to convert to/from linear/gamma.

class gamut {
    std::array<triplet, 3> f_mx;
    std::array<triplet, 3> r_mx;

    triplet xyz_w;

    triplet lum_mx;

    bool d65_ = false;

    void generate_matrices(const triplet& r, const triplet& g, const triplet& b, const triplet& w) {
        const auto xr = r.comp[0];
        const auto yr = r.comp[1];
        const auto zr = 1 - (xr + yr);
        const auto xg = g.comp[0];
        const auto yg = g.comp[1];
        const auto zg = 1 - (xg + yg);
        const auto xb = b.comp[0];
        const auto yb = b.comp[1];
        const auto zb = 1 - (xb + yb);

        const auto xw = w.comp[0];
        const auto yw = w.comp[1];
        const auto zw = 1 - (xw + yw);

        // XYZ -> RGB matrix, before scaling to white.

        const auto rx = yg*zb - yb*zg;
        const auto ry = xb*zg - xg*zb;
        const auto rz = xg*yb - xb*yg;
        const auto gx = yb*zr - yr*zb;
        const auto gy = xr*zb - xb*zr;
        const auto gz = xb*yr - xr*yb;
        const auto bx = yr*zg - yg*zr;
        const auto by = xg*zr - xr*zg;
        const auto bz = xr*yg - xg*yr;

        // White scaling factors. Dividing by yw scales the white luminance to unity, as
        // conventional.

        const auto rw = (rx*xw + ry*yw + rz*zw)/yw;
        const auto gw = (gx*xw + gy*yw + gz*zw)/yw;
        const auto bw = (bx*xw + by*yw + bz*zw)/yw;

        // XYZ -> RGB matrix, correctly scaled to white.

        r_mx[0].comp[0] = rx/rw;
        r_mx[0].comp[1] = ry/rw;
        r_mx[0].comp[2] = rz/rw;
        r_mx[1].comp[0] = gx/gw;
        r_mx[1].comp[1] = gy/gw;
        r_mx[1].comp[2] = gz/gw;
        r_mx[2].comp[0] = bx/bw;
        r_mx[2].comp[1] = by/bw;
        r_mx[2].comp[2] = bz/bw;

        // Invert the matrix for RGB -> XYZ

        const auto det =
            r_mx[0].comp[0]*(r_mx[1].comp[1]*r_mx[2].comp[2] - r_mx[2].comp[1]*r_mx[1].comp[2]) -
            r_mx[0].comp[1]*(r_mx[1].comp[0]*r_mx[2].comp[2] - r_mx[1].comp[2]*r_mx[2].comp[0]) +
            r_mx[0].comp[2]*(r_mx[1].comp[0]*r_mx[2].comp[1] - r_mx[1].comp[1]*r_mx[2].comp[0]);

        f_mx[0].comp[0] = (r_mx[1].comp[1]*r_mx[2].comp[2] - r_mx[2].comp[1]*r_mx[1].comp[2])/det;
        f_mx[0].comp[1] = (r_mx[0].comp[2]*r_mx[2].comp[1] - r_mx[0].comp[1]*r_mx[2].comp[2])/det;
        f_mx[0].comp[2] = (r_mx[0].comp[1]*r_mx[1].comp[2] - r_mx[0].comp[2]*r_mx[1].comp[1])/det;
        f_mx[1].comp[0] = (r_mx[1].comp[2]*r_mx[2].comp[0] - r_mx[1].comp[0]*r_mx[2].comp[2])/det;
        f_mx[1].comp[1] = (r_mx[0].comp[0]*r_mx[2].comp[2] - r_mx[0].comp[2]*r_mx[2].comp[0])/det;
        f_mx[1].comp[2] = (r_mx[1].comp[0]*r_mx[0].comp[2] - r_mx[0].comp[0]*r_mx[1].comp[2])/det;
        f_mx[2].comp[0] = (r_mx[1].comp[0]*r_mx[2].comp[1] - r_mx[2].comp[0]*r_mx[1].comp[1])/det;
        f_mx[2].comp[1] = (r_mx[2].comp[0]*r_mx[0].comp[1] - r_mx[0].comp[0]*r_mx[2].comp[1])/det;
        f_mx[2].comp[2] = (r_mx[0].comp[0]*r_mx[1].comp[1] - r_mx[1].comp[0]*r_mx[0].comp[1])/det;
    }

public:
    // Construct with CIE coordinates for RGB and white.
    // This is not part of the type which should have an empty constexpr constructor!
    explicit gamut(const triplet& r, const triplet& g, const triplet& b, const triplet& w) :
        xyz_w(w.comp[0]*w.comp[2]/w.comp[1], w.comp[2],
              (1 - w.comp[0] - w.comp[1])*w.comp[2]/w.comp[1]),
        lum_mx(r.comp[2], g.comp[2], b.comp[2]) {

        generate_matrices(r, g, b, w);

        constexpr triplet d65_white{0.95047, 1.0, 1.08883};

        d65_ = compare(xyz_w.comp[0], d65_white.comp[0], .001) &&
               compare(xyz_w.comp[1], d65_white.comp[1], .001) &&
               compare(xyz_w.comp[2], d65_white.comp[2], .001);
    }

    // Quick test if this is a d65 color space. (okLAB only works in d65 so this is used to avoid a
    // conversion).
    bool d65() const noexcept {
        return d65_;
    }

    // Return matrix coefficients that convert RGB to XYZ, (not scaled to 100).
    const std::array<triplet, 3>& rgb2xyz_matrix() const noexcept {
        return f_mx;
    }

    // Return matrix coefficients that convert XYZ (not scaled to 100), to RGB.
    const std::array<triplet, 3>& xyz2rgb_matrix() const noexcept {
        return r_mx;
    }

    // Convert linear RGB to XYZ. XYZ result is not scaled to 100.
    triplet rgb2xyz(const triplet& rgbt) const noexcept {
        return triplet(
                   vec::dot(rgbt.comp, f_mx[0].comp),
                   vec::dot(rgbt.comp, f_mx[1].comp),
                   vec::dot(rgbt.comp, f_mx[2].comp)
               );
    }

    // Convert XYZ to linear RGB. Does not expect XYZ coordinates to be scaled to 100.
    triplet xyz2rgb(const triplet& xyzt) const noexcept {
        return triplet(
                   vec::dot(xyzt.comp, r_mx[0].comp),
                   vec::dot(xyzt.comp, r_mx[1].comp),
                   vec::dot(xyzt.comp, r_mx[2].comp)
               );
    }

    // Convert RGB to weighted luminance
    double luminance(const triplet& rgbt) const noexcept {
        return vec::dot(rgbt.comp, lum_mx.comp);
    }

    // Return CIE Y coordinates for RGB
    const triplet& luminance_mx() const noexcept {
        return lum_mx;
    }

    // Return XYZ coordinates for white.
    const triplet& xyz_white() const noexcept {
        return xyz_w;
    }
};


////////////////////////////////////////////////////////////////////////////////////////////////////
// Predefined dci_p3_gamut gamut so that it constructs instantly and is all accessed as constants.
// See function comments in gamut class.
class dci_p3_gamut {
    constexpr static std::array<triplet, 3> f_mx = {
        triplet(0.4451698, 0.2771344, 0.1722827),
        triplet(0.2094917, 0.7215953, 0.0689131),
        triplet(0.0000000, 0.0470606, 0.9073554)
    };
    constexpr static std::array<triplet, 3> r_mx = {
        triplet(2.7253940, -1.0180030, -0.4401632),
        triplet(-0.7951680, 1.6897321, 0.0226472),
        triplet(0.0412419, -0.0876390, 1.1009294)
    };
    constexpr static triplet xyz_w = triplet(0.8945869, 1.0000000, 0.9544160);
    constexpr static triplet lum_mx = triplet(0.2094920, 0.7215950, 0.0689130);
public:
    constexpr bool d65() const {
        return false;
    }
    constexpr const std::array<triplet, 3>& rgb2xyz_matrix() const {
        return f_mx;
    }
    constexpr const std::array<triplet, 3>& xyz2rgb_matrix() const {
        return r_mx;
    }
    // Does not scale to 100.
    constexpr triplet rgb2xyz(const triplet& rgbt) const {
        return triplet(
                   vec::dot(rgbt.comp, f_mx[0].comp),
                   vec::dot(rgbt.comp, f_mx[1].comp),
                   vec::dot(rgbt.comp, f_mx[2].comp)
               );
    }
    // Expects normal XYZ (not scaled to 100).
    constexpr triplet xyz2rgb(const triplet& xyzt) const {
        return triplet(
                   vec::dot(xyzt.comp, r_mx[0].comp),
                   vec::dot(xyzt.comp, r_mx[1].comp),
                   vec::dot(xyzt.comp, r_mx[2].comp)
               );
    }
    constexpr double luminance(const triplet& rgbt) const {
        return vec::dot(rgbt.comp, lum_mx.comp);
    }
    constexpr const triplet& luminance_mx() const {
        return lum_mx;
    }
    constexpr const triplet& xyz_white() const {
        return xyz_w;
    }
};


////////////////////////////////////////////////////////////////////////////////////////////////////
// Predefined sRGB gamut so that it constructs instantly and is all accessed as constants. See
// function comments in gamut class. This srgb_gamut must exist even if this source is modified to
// hard code some other color space. It is used for okLAB and displayed HEX (HTML) color
// conversions.

class srgb_gamut {
    constexpr static std::array<triplet, 3> f_mx = {
        triplet(0.4124564, 0.3575761, 0.1804375),
        triplet(0.2126729, 0.7151522, 0.0721750),
        triplet(0.0193339, 0.1191920, 0.9503041)
    };
    constexpr static std::array<triplet, 3> r_mx = {
        triplet(3.2404542, -1.5371385, -0.4985314),
        triplet(-0.9692660, 1.8760108, 0.0415560),
        triplet(0.0556434, -0.2040259, 1.0572252)
    };
    constexpr static triplet xyz_w = triplet(0.95046, 1.00000, 1.08906);
    constexpr static triplet lum_mx = triplet(0.21266, 0.71516, 0.07219);
public:
    constexpr bool d65() const {
        return true;
    }

    constexpr const std::array<triplet, 3>& rgb2xyz_matrix() const {
        return f_mx;
    }

    constexpr const std::array<triplet, 3>& xyz2rgb_matrix() const {
        return r_mx;
    }

    // Does not scale to 100.
    constexpr triplet rgb2xyz(const triplet& rgbt) const {
        return triplet(
                   vec::dot(rgbt.comp, f_mx[0].comp),
                   vec::dot(rgbt.comp, f_mx[1].comp),
                   vec::dot(rgbt.comp, f_mx[2].comp)
               );
    }

    // Expects normal XYZ (not scaled to 100).
    constexpr triplet xyz2rgb(const triplet& xyzt) const {
        return triplet(
                   vec::dot(xyzt.comp, r_mx[0].comp),
                   vec::dot(xyzt.comp, r_mx[1].comp),
                   vec::dot(xyzt.comp, r_mx[2].comp)
               );
    }
    constexpr double luminance(const triplet& rgbt) const {
        return vec::dot(rgbt.comp, lum_mx.comp);
    }
    constexpr const triplet& luminance_mx() const {
        return lum_mx;
    }
    constexpr const triplet& xyz_white() const {
        return xyz_w;
    }
};


////////////////////////////////////////////////////////////////////////////////////////////////////
// Predefined adobe gamut so that it constructs instantly and is all accessed as constants. See
// function comments in gamut class.
class adobe_gamut {
    constexpr static std::array<triplet, 3> f_mx = {
        triplet(0.5766690, 0.1855582, 0.1882286), // NOLINT: warning confused with std numbers
        triplet(0.2973450, 0.6273636, 0.0752915),
        triplet(0.0270314, 0.0706889, 0.9913375)
    };
    constexpr static std::array<triplet, 3> r_mx = {
        triplet(2.0415879, -0.5650070, -0.3447314), // NOLINT: warning confused with std numbers
        triplet(-0.9692436, 1.8759675, 0.0415551),
        triplet(0.0134443, -0.1183624, 1.0151750)
    };
    constexpr static triplet xyz_w = triplet(0.9504559, 1.0000000, 1.0890578);
    constexpr static triplet lum_mx = triplet(0.2973610, 0.6273550, 0.0752850);
public:
    constexpr bool d65() const {
        return true;
    }

    constexpr const std::array<triplet, 3>& rgb2xyz_matrix() const {
        return f_mx;
    }

    constexpr const std::array<triplet, 3>& xyz2rgb_matrix() const {
        return r_mx;
    }

    // Does not scale to 100. This must be done after calling for scaled XYZ
    constexpr triplet rgb2xyz(const triplet& rgbt) const {
        return triplet(
                   vec::dot(rgbt.comp, f_mx[0].comp),
                   vec::dot(rgbt.comp, f_mx[1].comp),
                   vec::dot(rgbt.comp, f_mx[2].comp)
               );
    }

    // Does not scale to 1. This must be done before calling if input is scaled XYZ.
    constexpr triplet xyz2rgb(const triplet& xyzt) const {
        return triplet(
                   vec::dot(xyzt.comp, r_mx[0].comp),
                   vec::dot(xyzt.comp, r_mx[1].comp),
                   vec::dot(xyzt.comp, r_mx[2].comp)
               );
    }
    constexpr double luminance(const triplet& rgbt) const {
        return vec::dot(rgbt.comp, lum_mx.comp);
    }
    constexpr const triplet& luminance_mx() const {
        return lum_mx;
    }
    constexpr const triplet& xyz_white() const {
        return xyz_w;
    }
};


////////////////////////////////////////////////////////////////////////////////////////////////////
// These colorspaces construct instantly. The srgb one must exist.

struct srgb_colorspace {
    constexpr srgb_gamut gamut() const {
        return srgb_gamut();
    }

    double to_linear(double gamma) const {
        constexpr double threshold = .04045;
        constexpr double scale_low = 1/12.92;
        constexpr double scale_high = 1/1.055;
        constexpr double offset = .055;
        constexpr double curve = 2.4;

        if(gamma <= threshold) {
            return gamma*scale_low;
        }

        return std::pow((gamma + offset)*scale_high, curve);
    }

    double to_gamma(double linear) const {
        constexpr double threshold = .0031308;
        constexpr double scale_low = 12.92;
        constexpr double scale_high = 1.055;
        constexpr double offset = .055;
        constexpr double curve = 1/2.4;

        if(linear <= threshold) {
            return linear*scale_low;
        }

        return scale_high*std::pow(linear, curve) - offset;
    }
};

// This uses a simple 2.2 power law for the srgb tone curve.
struct srgb_22_colorspace {
    constexpr srgb_gamut gamut() const {
        return srgb_gamut();
    }

    double to_linear(double gamma) const {
        constexpr double curve = 2.2;

        return std::pow(gamma, curve);
    }

    double to_gamma(double linear) const {
        constexpr double curve = 1/2.2;

        return std::pow(linear, curve);
    }
};

// This uses a simple 2.4 power law for the srgb tone curve.
struct srgb_24_colorspace {
    constexpr srgb_gamut gamut() const {
        return srgb_gamut();
    }

    double to_linear(double gamma) const {
        constexpr double curve = 2.4;

        return std::pow(gamma, curve);
    }

    double to_gamma(double linear) const {
        constexpr double curve = 1/2.4;

        return std::pow(linear, curve);
    }
};

struct adobe_colorspace {
    constexpr adobe_gamut gamut() const {
        return adobe_gamut();
    }

    double to_linear(double gamma) const {
        constexpr double curve = 2.2;

        return std::pow(gamma, curve);
    }

    double to_gamma(double linear) const {
        constexpr double curve = 1/2.2;

        return std::pow(linear, curve);
    }
};

struct dci_p3_colorspace {
    constexpr dci_p3_gamut gamut() const {
        return dci_p3_gamut();
    }

    double to_linear(double gamma) const {
        constexpr double curve = 2.6;

        return std::pow(gamma, curve);
    }

    double to_gamma(double linear) const {
        constexpr double curve = 1/2.6;

        return std::pow(linear, curve);
    }
};

struct dci_p3_colorspace_24 {
    constexpr dci_p3_gamut gamut() const {
        return dci_p3_gamut();
    }

    double to_linear(double gamma) const {
        constexpr double curve = 2.4;

        return std::pow(gamma, curve);
    }

    double to_gamma(double linear) const {
        constexpr double curve = 1/2.4;

        return std::pow(linear, curve);
    }
};

// Currently the only method of changing the color space either for compile time or run time is to
// modify this definition. Variable color space support will be added if there's any interest.

using color_space = srgb_colorspace;


} // namespace cosel

#endif
