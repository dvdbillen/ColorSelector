// SPDX-License-Identifier: MIT
// Copyright (c) 2026 David Billen

// Hue-sat-L/V  color models collectively being called "HSx"
//
// This includes:
//    - Standard HSL
//    - Standard HSV
//    - HSL+ (hsl2) that is not standard. The L component is as in LUV and the saturation ranges
//      from none to the most possible for the selected hue in the gamut. The H value is not
//      consistent with the other HSx models.
//    - HSB+ (hsv2) that is not standard. The S component is modified to produce a saturation level
//      derived from LUV. This has a much more consistent saturation appearance for different hues
//      than the conventional gamma space calculation.
//
// All the classes use normals for all three components. Scaling is handled by UI.

// TODO: Decide when/how to initialize luv_gamut (right now it's a static in hsv2)

#ifndef cosel_model_hsx_h
#define cosel_model_hsx_h

#include <algorithm>
#include <cmath>
#include <iostream>

#include "cosel_luv_gamut.h"
#include "cosel_math.h"
#include "cosel_model.h"


namespace cosel {

///////////////////////////////////////////////////////////////////////////////////////////////////
// Base class for HSx models. Substitutes "B" (brightness) for L or V.

namespace details {

template<class IMPL>
class hsb_base {
public:
    [[nodiscard]] rgb to_rgb() const noexcept {
        return impl()->to_rgb_();
    }

    // Returns true if s and b would result in a completely desaturated color
    static bool is_gray(double s, double b) {
        return IMPL::is_gray_(s, b);
    }

    // Calculate only the hue of an RGB. This is an optimization in case the implementation can
    // derive only hue a lot faster than calculating S and B also.
    [[nodiscard]] static double calc_hue(const rgb& c) noexcept {
        return IMPL::calc_hue_(c);
    }

    bool is_gray() const {
        return impl()->is_gray_(s(), b());
    }

    const triplet& data() const {
        return data_;
    }

    double h() const {
        return data_.comp[0];
    }
    double s() const {
        return data_.comp[1];
    }
    double b() const {
        return data_.comp[2];
    }

    void set_h(double n) {
        data_.comp[0] = mod1(n);
    }
    void set_s(double n) {
        data_.comp[1] = clamp(n);
    }
    void set_b(double n) {
        data_.comp[2] = clamp(n);
    }

    [[nodiscard]] IMPL with_h(double n) const {
        return IMPL(mod1(n), data_.comp[1], data_.comp[2]);
    }
    [[nodiscard]] IMPL with_s(double n) const {
        return IMPL(data_.comp[0], clamp(n), data_.comp[2]);
    }
    [[nodiscard]] IMPL with_b(double n) const {
        return IMPL(data_.comp[0], data_.comp[1], clamp(n));
    }

    // Helper to return a vector of rgbs to be used as a source for making a hue gradient.
    static std::vector<rgb> make_gradient_source(double s, double b) {
        return std::vector<rgb> {
            IMPL(0/6.0, s, b).to_rgb(),
            IMPL(1/6.0, s, b).to_rgb(),
            IMPL(2/6.0, s, b).to_rgb(),
            IMPL(3/6.0, s, b).to_rgb(),
            IMPL(4/6.0, s, b).to_rgb(),
            IMPL(5/6.0, s, b).to_rgb(),
            IMPL(0/6.0, s, b).to_rgb()
        };
    }

protected:
    // TODO: This will need to be moved if codebase modified to support runtime color space
    // changes
    static const luv_gamut& luv_info() {
        static luv_gamut info;
        return info;
    }

private:
    friend IMPL;
    hsb_base()=default;
    hsb_base(const rgb& c_) noexcept {
        // The rgb color should be in range. The clamping is only intended to fix up any minor
        // slop from a conversion.
        data_ = impl()->from_rgb_(c_.clamped());
    }
    constexpr explicit hsb_base(double h_, double s_, double b_) noexcept :
        data_(mod1(h_), clamp(s_), clamp(b_)) {}
    explicit hsb_base(triplet t) noexcept : hsb_base(t.comp[0], t.comp[1], t.comp[2]) {}


    triplet data_;
    IMPL* impl() {
        return static_cast<IMPL*>(this);
    }
    const IMPL* impl() const {
        return static_cast<const IMPL*>(this);
    }
};

} // namespace details


///////////////////////////////////////////////////////////////////////////////////////////////////
// H S L

class hsl : public details::hsb_base<hsl> {
    friend class details::hsb_base<hsl>;
public:
    hsl()=default;
    constexpr explicit hsl(double h_, double s_, double l_) :
        details::hsb_base<hsl>(h_, s_, l_) {}
    explicit hsl(triplet t) : details::hsb_base<hsl>(t) {}
    explicit hsl(const rgb& c_) : details::hsb_base<hsl>(c_) {}

    double l() const {
        return b();
    }
    void set_l(double n) {
        set_b(n);
    }
    [[nodiscard]] hsl with_l(double n) const {
        return with_b(n);
    }

private:
    static double hue_to_rgb(double p, double q, double t) {
        t = mod1(t);

        if(t < 1.0/6) {
            return p + (q - p)*6*t;
        }

        if(t < 1.0/2) {
            return q;
        }

        if(t < 2.0/3) {
            return p + (q - p)*(2.0/3 - t)*6;
        }

        return p;
    }

    triplet from_rgb_(const rgb& c_) {
        const auto c = c_.to_gamma();
        const auto min = c.min();
        const auto max = c.max();
        const auto delta = max - min;

        // Calc L
        const auto l_ = (max + min)*.5;

        double h_ = 0;
        double s_ = 0;

        if(delta > zero_epsilon) {

            // Calc S
            s_ = (l_ < .5)? (delta/(max + min)): (delta/(2 - max - min));

            // Calc H
            if(max == c.r()) {
                h_ = (c.g() - c.b())/delta;
            } else if(max == c.g()) {
                h_ = 2 + (c.b() - c.r())/delta;
            } else {
                h_ = 4 + (c.r() - c.g())/delta;
            }

            h_ = mod1(h_/6);
        }

        return triplet{h_, s_, l_};
    }

    [[nodiscard]] static double calc_hue_(const rgb& c) noexcept {
        return c.hue();
    }

    [[nodiscard]] rgb to_rgb_() const {
        const auto h_ = mod1(h());
        const auto s_ = s();
        const auto l_ = l();

        if(s_ < zero_epsilon) {
            return rgb(l_, l_, l_).to_linear();
        }

        const auto q = (l_ < .5)? l_*(1 + s_): (l_ + s_ - l_*s_);
        const auto p = 2*l_ - q;

        const auto r = hue_to_rgb(p, q, h_ + (1.0/3));
        const auto g = hue_to_rgb(p, q, h_);
        const auto b = hue_to_rgb(p, q, h_ - (1.0/3));

        return rgb(r, g, b).cropped().to_linear();
    }

    static bool is_gray_(double s, double l) {
        return iszero(s) || iszero(l) || isone(l);
    }
};


///////////////////////////////////////////////////////////////////////////////////////////////////
// H S V

class hsv : public details::hsb_base<hsv> {
    friend class details::hsb_base<hsv>;
public:
    hsv()=default;
    constexpr explicit hsv(double h_, double s_, double v_) :
        details::hsb_base<hsv>(h_, s_, v_) {}
    explicit hsv(triplet t) : details::hsb_base<hsv>(t) {}
    explicit hsv(const rgb& c_) : details::hsb_base<hsv>(c_) {}

    double v() const {
        return b();
    }

    void set_v(double n) {
        set_b(n);
    }

    [[nodiscard]] hsv with_v(double n) const {
        return with_b(n);
    }

private:
    triplet from_rgb_(const rgb& c_) {
        const auto c = c_.to_gamma();
        const auto min = c.min();
        const auto max = c.max();
        const auto delta = max - min;

        double h_ = 0;
        double s_ = 0;

        // Calc V
        const auto v_ = max;

        if(max > zero_epsilon) {

            // Calc S
            s_ = delta / max;

            // Calc H
            if(delta == 0) {
                h_ = 0;
            } else if(max == c.r()) {
                h_ = (c.g() - c.b())/delta;
            } else if(max == c.g()) {
                h_ = 2 + (c.b() - c.r())/delta;
            } else {
                h_ = 4 + (c.r() - c.g())/delta;
            }

            h_ = mod1(h_/6);
        }

        return triplet{h_, s_, v_};
    }

    [[nodiscard]] static double calc_hue_(const rgb& c) noexcept {
        return c.hue();
    }

    [[nodiscard]] rgb to_rgb_() const {
        const auto h_ = h();
        const auto s_ = s();
        const auto v_ = v();

        if(is_gray(s_, v_)) {
            return rgb(v_, v_, v_).to_linear();
        }

        const auto h_sector = h_*6;
        const auto i = int(h_sector);
        const auto f = h_sector - i;
        const auto p = v_*(1 - s_);
        const auto q = v_*(1 - s_*f);
        const auto t = v_*(1 - s_*(1 - f));

        switch(i) {
            case 0:
                return rgb(v_, t, p).cropped().to_linear();

            case 1:
                return rgb(q, v_, p).cropped().to_linear();

            case 2:
                return rgb(p, v_, t).cropped().to_linear();

            case 3:
                return rgb(p, q, v_).cropped().to_linear();

            case 4:
                return rgb(t, p, v_).cropped().to_linear();

            default:
                return rgb(v_, p, q).cropped().to_linear();
        }
    }

    static bool is_gray_(double s, double v) {
        return iszero(s) || iszero(v);
    }

    // This converts a hue to linear. It is a useless function outside of our application.
    [[nodiscard]] static double linear_hue(double hue) {
        const auto c = hsv(hue, 1, 1).to_rgb();
        const auto max = c.max();

        double h = 0;

        if(max == c.r()) {
            h = c.g() - c.b();
        } else if(max == c.g()) {
            h = 2 + c.b() - c.r();
        } else {
            h = 4 + c.r() - c.g();
        }

        return mod1(h/6);
    }
};


///////////////////////////////////////////////////////////////////////////////////////////////////
// H S V +
// Non-standard HSV that produces a more perceptually consistent saturation level
//
// H: Same as HSV
// S: Derived from LUV colorspace
// V: Same as HSV

class hsv2 : public details::hsb_base<hsv2> {
    friend class details::hsb_base<hsv2>;
public:
    hsv2()=default;
    constexpr explicit hsv2(double h_, double s_, double v_) :
        details::hsb_base<hsv2>(h_, s_, v_) {}
    explicit hsv2(triplet t) : details::hsb_base<hsv2>(t) {}
    explicit hsv2(const rgb& c_) : details::hsb_base<hsv2>(c_) {}

    double v() const {
        return b();
    }

    void set_v(double n) {
        set_b(n);
    }

    [[nodiscard]] hsv2 with_v(double n) const {
        return with_b(n);
    }

private:
    triplet from_rgb_(const rgb& c_) {

        const auto& luvinfo = luv_info();

        // Calc V
        const auto v_ = clamp(c_.to_gamma().max());

        if(c_.is_gray()) {
            return triplet{0, 0, v_};
        }

        // Calc S

        // This is determined by the color distance from gray divided by distance to edge-of-gamut
        // (pure chromna) in LUV color space. It always uses the same LUV L value that is below
        // blue level where we have pregenerated the LUV RGB triangle for the color space (via
        // luvinfo.lgamut)

        const auto rgb_at_l_level = c_.forced(L_to_y(luvinfo.safe_L()));
        const auto luv_sat = luv(rgb_at_l_level);
        const auto luv_sat_uv = luv_sat.uv();
        const auto luv_pure_uv = luvinfo.gamut_max(luv_sat_uv);
        const auto s_ = clamp(luv_sat_uv.distance()/luv_pure_uv.distance());

        // Calc H

        // The hue value needs to be based off the "pure" LUV (full chroma) value used to calculate
        // saturation rather than the original input color. This avoids small differences between
        // from_rgb and to_rgb

        const auto luv_pure = luv(luvinfo.safe_L(), luv_pure_uv);
        const auto rgb_pure = rgb(luv_pure).cropped_negative().normalized().to_gamma();

        const auto min = rgb_pure.min();
        const auto max = rgb_pure.max();
        const auto delta = max - min;

        double h_ = 0;

        if(delta == 0) {
            h_ = 0;
        } else if(max == rgb_pure.r()) {
            h_ = (rgb_pure.g() - rgb_pure.b())/delta;
        } else if(max == rgb_pure.g()) {
            h_ = 2 + (rgb_pure.b() - rgb_pure.r())/delta;
        } else {
            h_ = 4 + (rgb_pure.r() - rgb_pure.g())/delta;
        }

        h_ = mod1(h_/6);

        return triplet{h_, s_, v_};
    }

    [[nodiscard]] static double calc_hue_(const rgb& c_) noexcept {

        // This may not shave enough time off a full conversion to be worth duplicating part of the
        // logic. TODO: Compare speed against: return from_rgb_(c_).comp[0];

        const auto& luvinfo = luv_info();

        if(c_.is_gray()) {
            return 0;
        }

        const auto rgb_at_l_level = c_.forced(L_to_y(luvinfo.safe_L()));
        const auto luv_sat = luv(rgb_at_l_level);
        const auto luv_sat_uv = luv_sat.uv();
        const auto luv_pure_uv = luvinfo.gamut_max(luv_sat_uv);

        // The hue value needs to be based off the "pure" LUV (full chroma) value used to calculate
        // saturation rather than the original input color. This avoids small differences between
        // from_rgb and to_rgb

        const auto luv_pure = luv(luvinfo.safe_L(), luv_pure_uv);
        const auto rgb_pure = rgb(luv_pure).cropped_negative().normalized().to_gamma();

        const auto min = rgb_pure.min();
        const auto max = rgb_pure.max();
        const auto delta = max - min;

        double h_ = 0;

        if(delta == 0) {
            h_ = 0;
        } else if(max == rgb_pure.r()) {
            h_ = (rgb_pure.g() - rgb_pure.b())/delta;
        } else if(max == rgb_pure.g()) {
            h_ = 2 + (rgb_pure.b() - rgb_pure.r())/delta;
        } else {
            h_ = 4 + (rgb_pure.r() - rgb_pure.g())/delta;
        }

        return mod1(h_/6);
    }

    [[nodiscard]] rgb to_rgb_() const {
        if(is_gray(s(), v())) {
            return rgb(v(), v(), v()).to_linear();
        }

        const auto& luvinfo = luv_info();

        const auto h_sector = h()*6;
        const auto index = int(h_sector);
        const auto f = h_sector - index;

        rgb rgb_c;

        switch(index) {
            case 0:
                rgb_c = rgb(1, f, 0);
                break;

            case 1:
                rgb_c = rgb(1 - f, 1, 0);
                break;

            case 2:
                rgb_c = rgb(0, 1, f);
                break;

            case 3:
                rgb_c = rgb(0, 1 - f, 1);
                break;

            case 4:
                rgb_c = rgb(f, 0, 1);
                break;

            default:
                rgb_c = rgb(1, 0, 1 - f);
                break;
        }

        // Make the result linear (after clamping to remove any precision noise)
        const auto rgb_clamped = rgb_c.clamped().to_linear();
        // Force it to the level we use for LUV saturation calculations
        const auto rgb_scaled = rgb_clamped.forced(L_to_y(luvinfo.safe_L()));
        // Put it in LUV
        const auto luv_pure = luv(rgb_scaled);
        // It was already at edge of gamut - simply scale by the S value
        const auto luv_sat = luv_pure.with_uv(luv_pure.uv()*s());

        // Get V as linear so it can be used to scale a linear RGB result...
        const auto linear_v = xyz::rgb_to_linear(v());

        // Convert the LUV value to RGB, normalize it, and scale by V
        return rgb(luv_sat).normalized()*linear_v;
    }

    static bool is_gray_(double s, double v) {
        return iszero(s) || iszero(v);
    }

    // This converts a hue to linear. It is a useless function outside of our application.
    [[nodiscard]] static double linear_hue(double hue) {
        const auto c = hsv(hue, 1, 1).to_rgb();
        const auto max = c.max();

        double h = 0;

        if(max == c.r()) {
            h = c.g() - c.b();
        } else if(max == c.g()) {
            h = 2 + c.b() - c.r();
        } else {
            h = 4 + c.r() - c.g();
        }

        return mod1(h/6);
    }
};



///////////////////////////////////////////////////////////////////////////////////////////////////
// H S L +
// Non-standard HSL that produces more consistent levels
//
// H: Works like HSL but values are not identical except for the primaries and secondaries
// S: Amount of saturation from gray to the most saturated the hue can be at the L setting
// L: LAB L*.01

class hsl2 : public details::hsb_base<hsl2> {
    friend class details::hsb_base<hsl2>;
public:
    hsl2() = default;
    explicit hsl2(triplet t) : details::hsb_base<hsl2>(t) {}
    explicit hsl2(double h, double s, double l) noexcept :
        details::hsb_base<hsl2>(h, s, l) {}
    explicit hsl2(const rgb& rgb_c) noexcept : details::hsb_base<hsl2>(rgb_c) {}

    double l() const {
        return b();
    }
    void set_l(double n) {
        set_b(n);
    }
    hsl2 with_l(double n) const {
        return with_b(n);
    }
private:
    triplet from_rgb_(const rgb& rgb_c) noexcept {
        const auto& luvinfo = luv_info();
        const auto luminance = rgb_c.level();

        if(rgb_c.is_gray()) {
            return triplet{0, 0, y_to_L(luminance)/100};
        }

        if(luminance < zero_epsilon || luminance > (1 - zero_epsilon)) {
            return triplet{0, 0, luminance};
        }

        const auto luv_c = luv(rgb_c);

        // Calc H

        // Use LUV polar coordinate at gamuts edge. The lgamut angle_to_hue function contrives it
        // so that the primaries and secondaries line up with conventional HSL to be familiar in
        // usage if not entirely compatible

        const auto angle = std::atan2(luv_c.v(), luv_c.u());
        const auto h_ = luvinfo.angle_to_hue(angle);

        // Calc S
        const auto rgb_max = rgb_c.maxxed_gamut();
        const auto luv_max = luv(rgb_max);
        const auto s_ = clamp(luv_c.distance_from_gray()/luv_max.distance_from_gray());

        // Calc L
        const auto l_ = y_to_L(luminance)/100;
        return triplet{h_, s_, l_};
    }

    [[nodiscard]] static double calc_hue_(const rgb& rgb_c) noexcept {

        if(rgb_c.is_gray()) {
            return 0;
        }

        const auto luminance = rgb_c.level();

        if(luminance < zero_epsilon || luminance > (1 - zero_epsilon)) {
            return 0;
        }

        const auto luv_c = luv(rgb_c);
        const auto angle = std::atan2(luv_c.v(), luv_c.u());
        return luv_info().angle_to_hue(angle);
    }

    rgb to_rgb_() const noexcept {
        const auto& luvinfo = luv_info();
        const auto luminance = L_to_y(l()*100);

        if(s() <= zero_epsilon || l() <= zero_epsilon) {
            return rgb(luminance, luminance, luminance);
        }

        // The reverse of angle_to_hue supporting the contrived primary and secondary positions
        const auto angle = luvinfo.hue_to_angle(h());

        // Get an RGB in the darker full gamut range (via LUV). Using the dark level lets us get a
        // full chroma color down where the gamut is a full RGB triangle (and the angle is
        // therefore accurate)
        const auto dark_luv = luvinfo.safe_luv(angle);
        const auto dark_rgb = rgb(dark_luv);
        // Force it to specified brightness.
        const auto bright_rgb = dark_rgb.forced(L_to_y(l()*100));
        // Force to edge of gamut (max chroma)
        const auto adjusted_rgb = bright_rgb.in_gamut() ? bright_rgb.maxxed_gamut()
                                  : bright_rgb.clamped_gamut();
        // Put it back to LUV
        const auto pure_luv = luv{adjusted_rgb};
        const auto sat_luv = pure_luv.scaled_uv(s());

        const auto ret = rgb(sat_luv).clamped_gamut();
        return ret;
    }

    static bool is_gray_(double s, double l) {
        return iszero(s) || iszero(l) || isone(l);
    }

};


} // namespace cosel


#endif // cosel_model_hsx_h
