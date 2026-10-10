// SPDX-License-Identifier: MIT
// Copyright (c) 2026 David Billen

// rgb_selector

#ifndef cosel_qt_select_group_rgb_h
#define cosel_qt_select_group_rgb_h

#include "cosel_qt_select_group_base.h"

namespace cosel {

namespace details {

// Fast linear gradient for rgb colors. src_ and dst_ should be gamma corrected (non-linear) when
// passed to any of the functions. (These sliders maintain the current color setting non-linear).
//
// Note: Uniform gradients for RGB components were tried and discarded because they affect the knob
// position of the sliders. RGB is more color 'mixing' than 'selecting' and the relative position
// of the three knobs is critical.
class rgb_gradient_t : public gradient_base_t {
public:
    ~rgb_gradient_t() override = default;

    // src_ and dst_ should be non-linear
    explicit rgb_gradient_t(const rgb& src_, const rgb& dst_, size_t n) {
        set(src_, dst_, n);
    }

    size_t size() const override {
        return gradient.size();
    }

    bool empty() const override {
        return gradient.empty();
    }

    // src_ and dst_ should be non-linear
    void set(const rgb& src_, const rgb& dst_, size_t size) {
        src = src_;
        dst = dst_;
        rgb_gradient_t::resize(size);
    }

    // src_ and dst_ should be non-linear
    void set_color(const rgb& src_, const rgb& dst_) {
        set(src_, dst_, gradient.size());
    }

    void resize(size_t n) override {
        gradient.clear();
        gradient.reserve(n);

        const auto inc = 1.0/(n - 1);

        for(unsigned i = 0; i < n; ++i) {
            const auto c = src.interpolated(dst, i*inc);
            gradient.emplace_back(c.to_linear());
        }
    }

    // The results are linear as expected
    void get_rgbs(std::vector<rgb>& results) const override {
        results.clear();
        results.assign(gradient.begin(), gradient.end());
    }

    double source_to_gradient(double source_pos) const override {
        return source_pos;
    }

    double gradient_to_source(double gradient_pos) const override {
        return gradient_pos;
    }

private:
    std::vector<rgb> gradient;
    rgb src;
    rgb dst;
};


} // namespace details


class rgb_selector : public color_selector {
public:
    explicit rgb_selector(selector_type_t type, const selected_color_t& color,
                          const gradient_slider::style_options_t& options_) :
        color_selector(type, color, options_) {
        initialize();
    }

private:
    using gradient_t = details::rgb_gradient_t;

    constexpr static double range = 255;

    rgb rgb_color;

    gradient_slider* red = nullptr;
    gradient_slider* green = nullptr;
    gradient_slider* blue = nullptr;

    std::shared_ptr<gradient_t> red_gradient;
    std::shared_ptr<gradient_t> green_gradient;
    std::shared_ptr<gradient_t> blue_gradient;

    void initialize();
    void handle_color_change() override;
    void handle_slider_change(int which_slider);
    void rethink_gradients(int unchanged_slider);
};

inline void rgb_selector::initialize() {
    rgb_color = selection_->rgb().to_gamma();

    const auto& info = get_selector_info(selector_type_t::rgb);

    auto* rgrad = new gradient_t(rgb(0, 0, 0), rgb(1, 0, 0), 50);
    red_gradient = std::shared_ptr<gradient_t>(rgrad);
    auto* ggrad = new gradient_t(rgb(0, 0, 0), rgb(0, 1, 0), 50);
    green_gradient = std::shared_ptr<gradient_t>(ggrad);
    auto* bgrad = new gradient_t(rgb(0, 0, 0), rgb(0, 0, 1), 50);
    blue_gradient = std::shared_ptr<gradient_t>(bgrad);

    const auto names = info.component_names;

    red = new gradient_slider(names[0], 0, range, options, rgb_color.r()*range);
    red->set_gradient(red_gradient);
    connect(red, &gradient_slider::changed, this, [this]() {
        handle_slider_change(0);
    });

    green = new gradient_slider(names[1], 0, range, options, rgb_color.g()*range);
    green->set_gradient(green_gradient);
    connect(green, &gradient_slider::changed, this, [this]() {
        handle_slider_change(1);
    });

    blue = new gradient_slider(names[2], 0, range, options, rgb_color.b()*range);
    blue->set_gradient(blue_gradient);
    connect(blue, &gradient_slider::changed, this, [this]() {
        handle_slider_change(2);
    });

    auto* layout = make_slider_layout({red, green, blue});
    setLayout(layout);
}

inline void rgb_selector::handle_color_change() {
    rgb_color = selection_->rgb().to_gamma();

    auto r = rgb_color.r();
    auto g = rgb_color.g();
    auto b = rgb_color.b();

    red->set_value(r*range);
    green->set_value(g*range);
    blue->set_value(b*range);

    rethink_gradients(-1);
}

inline void rgb_selector::handle_slider_change(int which_slider) {
    auto r = red->value()/range;
    auto g = green->value()/range;
    auto b = blue->value()/range;

    rgb_color = rgb(r, g, b);
    selection_->set_rgb(rgb_color.to_linear(), color_id);

    rethink_gradients(which_slider);
}

inline void rgb_selector::rethink_gradients(int unchanged_slider) {
    if(unchanged_slider != 0) {
        red_gradient->set(rgb_color.with_r(0), rgb_color.with_r(1),
                          red->gradient()->size());
        red->set_gradient(red_gradient);
    }

    if(unchanged_slider != 1) {
        green_gradient->set(rgb_color.with_g(0), rgb_color.with_g(1),
                            green->gradient()->size());
        green->set_gradient(green_gradient);
    }

    if(unchanged_slider != 2) {
        blue_gradient->set(rgb_color.with_b(0), rgb_color.with_b(1),
                           blue->gradient()->size());
        blue->set_gradient(blue_gradient);
    }
}



} // namespace cosel


#endif // cosel_qt_select_group_rgb_h

