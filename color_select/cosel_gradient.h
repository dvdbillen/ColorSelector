// SPDX-License-Identifier: MIT
// Copyright (c) 2026 David Billen

// Gradient generation with abstract base class

#ifndef cosel_gradient_h
#define cosel_gradient_h

#include <algorithm>
#include <functional>
#include <stdexcept>
#include <vector>

#include "cosel_model.h"
#include "cosel_model_hsx.h"
#include "cosel_util.h"


namespace cosel {

// Base class that supports gradients that distinguish "source" (logical/linear) positions from
// "gradient" (rendered/perceptually uniform) positions.
//
// - Source position (0..1): logical position in the original color parameter space (e.g. hue 0.0 =
//   red, 1/6 = yellow, 2/6 = green, etc.).
//
// - Gradient position (0..1): actual position in the generated gradient buffer, which may use
//   non-linear spacing for better perceptual uniformity.
//
// Color sliders operate on gradient positions (what the user sees and points to), while
// application logic works with source positions, (value of color component). The conversion
// functions allow mapping between the two.

struct gradient_base_t {
    gradient_base_t() = default;
    virtual ~gradient_base_t() = default;

    virtual size_t size() const = 0;
    virtual bool empty() const = 0;
    virtual void resize(size_t n) = 0;

    // Returns the RGB values of the generated gradient (size() elements).
    virtual void get_rgbs(std::vector<rgb>& results) const = 0;

    // Convert between logical source position and rendered gradient position.
    virtual double source_to_gradient(double source_pos) const = 0;
    virtual double gradient_to_source(double gradient_pos) const = 0;
};


///////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
// HSx Hue gradient
//
// Gradient generated for the hue in an HSL/HSV types where the colors are affected by SL or SV.
// This currently uses okLAB to generate a perceptually uniform gradient to visually optimize the
// color selection process for the end user.
///////////////////////////////////////////////////////////////////////////////////////////////////

template<typename HSB>
class hsx_hue_gradient_t : public gradient_base_t {
    static_assert(std::is_same_v<HSB, hsv> || std::is_same_v<HSB, hsl> ||
                  std::is_same_v<HSB, hsv2> || std::is_same_v<HSB, hsl2>,
                  "template argument must be either hsv, hsv2, hsl, or hsl2");
public:
    hsx_hue_gradient_t() = default;
    hsx_hue_gradient_t(double s_, double b_, size_t n) :
        s(s_),
        b(b_),
        size_(n) {
        make_gradient();
    }

    void set(double s_, double b_, size_t n) {
        s = s_;
        b = b_;
        size_ = n;
        make_gradient();
    }

    void resize(size_t n) override {
        set(s, b, n);
    }

    size_t size() const override {
        return size_;
    }

    bool empty() const override {
        return size_ < 1;
    }

    void get_rgbs(std::vector<rgb>& results) const override {
        if(!gray) {
            results = rgb_colors;
            return;
        }

        // No color, return all gray
        results.clear();
        results.reserve(size_);
        const auto rgb_c = HSB(0, s, b).to_rgb();

        for(unsigned i = 0; i < size_; ++i) {
            results.emplace_back(rgb_c);
        }
    }

    // Convert from a source position 0 to 1 to a gradient position 0 to 1.
    double source_to_gradient(double source_pos) const override {
        source_pos = clamp(source_pos);

        if(gray || hue_xref.size() <= 1) {
            return source_pos;
        }

        const auto x = std::lower_bound(hue_xref.begin(), hue_xref.end(), source_pos);

        if(x == hue_xref.begin()) {
            return 0;
        }

        if(x == hue_xref.end()) {
            return 1;
        }

        const auto index = std::distance(hue_xref.begin(), x);
        const auto prev = index - 1;
        const auto h_prev = hue_xref[prev];
        const auto h_next = hue_xref[index];
        const auto delta  = h_next - h_prev;

        if(delta <= 0.0) {
            return double(prev)/(hue_xref.size() - 1);
        }

        assert(source_pos >= h_prev);
        const auto frac = (source_pos - h_prev)/delta;
        return (prev + frac)/(hue_xref.size() - 1);
    }

    // Convert from a gradient position 0 to 1 to a source position 0 to 1.
    double gradient_to_source(double gradient_pos) const override {
        gradient_pos = clamp(gradient_pos);

        if(gray || hue_xref.empty()) {
            return gradient_pos;
        }

        const auto n = hue_xref.size();

        if(n == 1) {
            return hue_xref[0];
        }

        const auto findex = gradient_pos*(n - 1);
        const auto index = size_t(findex);

        if(index >= n - 1) {
            return hue_xref.back();
        }

        const auto frac = findex - index;
        return lerp(hue_xref[index], hue_xref[index + 1], frac);
    }

private:
    double s = 1;
    double b = .5;
    size_t size_ = 0;
    bool gray = true;

    std::vector<double> hue_xref;
    std::vector<rgb> rgb_colors;

    // Scratch vectors for arc-length parameterization
    std::vector<oklab> irregular_in;
    std::vector<oklab> uniform_out;
    std::vector<double> arclen;

    // Arc-length parameterization (ish) function used to evenly distributes a vector of colors.
    void make_uniform() {
        uniform_out.clear();

        const auto size = irregular_in.size();

        if(size < 2) {
            uniform_out = irregular_in;
            return;
        }

        // Get the distance from color 0 of each color to a vector

        arclen.resize(size, 0.0);

        for(unsigned i = 1; i < size; ++i) {
            const auto distance = irregular_in[i - 1].delta(irregular_in[i]);
            arclen[i] = arclen[i - 1] + distance;
        }

        // Calculate the increment for a uniform distribution

        const auto total = arclen.back();
        const auto step = total/(size - 1);

        // Generate the uniform distribution by seeking the uniform positions and interpolating

        uniform_out.reserve(size);
        uniform_out.emplace_back(irregular_in.front());

        size_t seg = 0;

        for(unsigned i = 1; i < size - 1; ++i) {
            const auto target = i*step;

            while(seg + 1 < size - 1 && arclen[seg + 1] < target) {
                ++seg;
            }

            const auto seg_length = arclen[seg + 1] - arclen[seg];
            const auto amt = (seg_length > 0)? (target - arclen[seg])/seg_length : 0.0;
            uniform_out.emplace_back(irregular_in[seg].interpolated(irregular_in[seg + 1], amt));
        }

        uniform_out.emplace_back(irregular_in.back());
    }

    void make_gradient() {
        gray = HSB::is_gray(s, b);

        if(gray) {
            return;
        }

        // Convert size_ number of hues to okLAB and write to the irregular_in vector

        irregular_in.clear();
        irregular_in.reserve(size_);

        const auto step = 1.0/size_;

        for(unsigned i = 0; i < size_; ++i) {
            const auto hue = i*step;
            const auto hsb_c = HSB(hue, s, b);
            const auto rgb_c = hsb_c.to_rgb();
            const auto oklab_c = oklab(rgb_c);
            irregular_in.emplace_back(oklab_c);
        }

        // Convert irregular_in (perceptually irregular differences) to uniform_out (perceptually
        // uniform differences)

        make_uniform();

        // Build an xref of hues for use with gradient_to_source() and source_to_gradient() while
        // also building the vector of rgb colors for rendering the gradient.

        rgb_colors.clear();
        rgb_colors.reserve(size_);
        hue_xref.resize(size_);

        const auto rgb_0 = rgb(uniform_out[0]);
        rgb_colors.emplace_back(rgb_0.clamped_gamut());
        hue_xref[0] = 0;

        for(unsigned i = 1; i < size_; ++i) {
            const auto rgb_c = rgb(uniform_out[i]);
            // save clamped to gamut for display...
            rgb_colors.emplace_back(rgb_c.clamped_gamut());

            //...but use clamp which scales to range to best preserve hue for the xref
            const auto rgb_clamped = rgb_c.clamped();
            auto h = HSB::calc_hue(rgb_clamped);

            const auto prev_h = hue_xref[i - 1];
            const auto delta = h - prev_h;
            if(delta > .5) {
                h -= 1;
            } else if(delta < -.5) {
                h += 1;
            }

            h = std::max(h, prev_h);

            hue_xref[i] = h;
        }

        hue_xref.back() = 1;
    }

};


///////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
// Linear gradient
//
// A linear gradient has a 1 to 1 correlation between source and gradient positions and only needs
// a transform to convert a position to a result.

template<typename COLOR_TYPE>
class linear_gradient_t : public gradient_base_t {
    using func_t = std::function<rgb(const COLOR_TYPE& c, double pos)>;
public:
    linear_gradient_t() = default;
    ~linear_gradient_t() override = default;

    explicit linear_gradient_t(const COLOR_TYPE& c_, size_t n, func_t func_) :
        func(std::move(func_)) {
        set(c_, n);
    }

    size_t size() const override {
        return gradient_.size();
    }

    bool empty() const override {
        return gradient_.empty();
    }

    void set(const COLOR_TYPE& c_, size_t size) {
        color = c_;
        linear_gradient_t::resize(size);
    }

    void set_color(const COLOR_TYPE& c_) {
        set(c_, gradient_.size());
    }

    void resize(size_t n) override {
        gradient_.clear();
        gradient_.reserve(n);

        const auto inc = 1.0/(n - 1);

        for(unsigned i = 0; i < n; ++i) {
            const auto c = func(color, i*inc);
            gradient_.emplace_back(c);
        }
    }

    void get_rgbs(std::vector<rgb>& results) const override {
        results.clear();
        results.assign(gradient_.begin(), gradient_.end());
    }

    double source_to_gradient(double source_pos) const override {
        return source_pos;
    }

    double gradient_to_source(double gradient_pos) const override {
        return gradient_pos;
    }

private:
    std::vector<rgb> gradient_;
    COLOR_TYPE color;
    func_t func;
};


///////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
// L gradient
//
// An l_gradient_t generates a colorized LAB/LUV L gradient
///////////////////////////////////////////////////////////////////////////////////////////////////

class l_gradient_t : public gradient_base_t {
public:
    l_gradient_t() = default;
    ~l_gradient_t() override = default;

    // Specified color will be normalized
    explicit l_gradient_t(const rgb& color_, size_t n) :
        color(color_.normalized()) {
        set_ranges();
        l_gradient_t::resize(n);
    }

    size_t size() const override {
        return gradient_.size();
    }

    bool empty() const override {
        return gradient_.empty();
    }

    // The color should be at the max brightness
    void set_color(const rgb& color_) {
        if(color_ != color) {
            color = color_;
            set_ranges();
            resize(gradient_.size());
        }
    }

    void resize(size_t n) override {
        n = std::max<size_t>(n, 4);

        gradient_.clear();
        gradient_.reserve(n);

        const auto g_color = color.to_gamma();
        const auto inc = 1.0/(n - 1);

        gradient_.emplace_back(0, 0, 0);

        for(unsigned i = 1; i < n - 1; ++i) {
            const auto gradient_pos = i*inc;
            const auto source_pos = l_gradient_t::gradient_to_source(gradient_pos);
            gradient_.emplace_back((g_color*source_pos).to_linear());
        }

        gradient_.emplace_back(color);
    }

    void get_rgbs(std::vector<rgb>& results) const override {
        results.clear();
        results.assign(gradient_.begin(), gradient_.end());
    }

    double source_to_gradient(double source_pos) const override {
        source_pos = xyz::rgb_to_linear(source_pos*gamma_range);
        const auto gradient_pos = y_to_L(source_pos)*il_range;

        return clamp(gradient_pos);
    }

    double gradient_to_source(double gradient_pos) const override {
        auto source_pos = L_to_y(gradient_pos*l_range);
        source_pos = xyz::rgb_to_gamma(source_pos)*igamma_range;

        return clamp(source_pos);
    }

private:
    std::vector<rgb> gradient_;
    rgb color;

    double gamma_range = 1;
    double l_range = 100;
    double igamma_range = 1;
    double il_range = .01;

    void set_ranges() {
        const auto linear_range = color.level();
        // rgb::normalize returns 1,1,1 in the case of black, so this is just a paranoid check
        assert(linear_range > zero_epsilon);

        gamma_range = xyz::rgb_to_gamma(linear_range);
        l_range = y_to_L(linear_range);
        igamma_range = 1/gamma_range;
        il_range = 1/l_range;
    }
};


}

#endif // cosel_gradient_h
