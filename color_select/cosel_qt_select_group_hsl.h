// SPDX-License-Identifier: MIT
// Copyright (c) 2026 David Billen

// hsl_selector

#ifndef cosel_qt_select_group_hsl_h
#define cosel_qt_select_group_hsl_h

#include "cosel_qt_select_group_base.h"

namespace cosel {

class hsl_selector : public color_selector {
public:
    explicit hsl_selector(selector_type_t type, const selected_color_t& color,
                          const gradient_slider::style_options_t& options_) :
        color_selector(type, color, options_) {
        initialize();
    }

private:
    // Hue gradient is an hsx_hue_gradient_t<hsl>

    using hue_gradient_t = hsx_hue_gradient_t<hsl>;

    // Sat and L gradients are linear gradients

    class sat_gradient_t : public linear_gradient_t<hsl> {
    public:
        explicit sat_gradient_t(const hsl& c_, size_t n) :
            linear_gradient_t<hsl>(c_, n, [](const hsl& c, double pos) {
            return c.with_s(pos).to_rgb();
        }) {}

    };

    class hsll_gradient_t : public linear_gradient_t<hsl> {
    public:
        explicit hsll_gradient_t(const hsl& c_, size_t n) :
            linear_gradient_t<hsl>(c_, n, [](const hsl& c, double pos) {
            return c.with_l(pos).to_rgb();
        }) {}

    };

    constexpr static double hue_slider_range = 359;
    constexpr static double hue_color_range = 360;
    constexpr static double sat_range = 100;
    constexpr static double lvl_range = 100;

    constexpr static double hue_color_irange = 1/hue_color_range;
    constexpr static double sat_irange = 1/sat_range;
    constexpr static double lvl_irange = 1/lvl_range;

    hsl hsl_color;

    gradient_slider* hue = nullptr;
    gradient_slider* sat = nullptr;
    gradient_slider* level = nullptr;

    std::shared_ptr<hue_gradient_t> hue_gradient;
    std::shared_ptr<sat_gradient_t> sat_gradient;
    std::shared_ptr<hsll_gradient_t> level_gradient;

    void initialize();
    void handle_color_change() override;
    void handle_slider_change(int which_slider);
    void rethink_gradients(int unchanged_slider);
};

inline void hsl_selector::initialize() {
    hsl_color = hsl(selection_->rgb());

    auto* hgrad = new hue_gradient_t(1, .5, 50);
    hue_gradient = std::shared_ptr<hue_gradient_t>(hgrad);
    auto* sgrad = new sat_gradient_t(hsl(1, 1, 1), 50);
    sat_gradient = std::shared_ptr<sat_gradient_t>(sgrad);
    auto* lgrad = new hsll_gradient_t(hsl(1, 0, 1), 50);
    level_gradient = std::shared_ptr<hsll_gradient_t>(lgrad);

    const auto& info = get_selector_info(selector_type_t::hsl);
    const auto& names = info.component_names;

    hue = new gradient_slider(names[0], 0, hue_slider_range, options, hsl_color.h()*hue_color_range);
    hue->set_gradient(hue_gradient);

    sat = new gradient_slider(names[1], 0, sat_range, options, hsl_color.s()*sat_range);
    sat->set_gradient(sat_gradient);

    level = new gradient_slider(names[2], 0, lvl_range, options, hsl_color.l()*lvl_range);
    level->set_gradient(level_gradient);

    connect(hue, &gradient_slider::changed, this, [this]() {
        handle_slider_change(0);
    });
    connect(sat, &gradient_slider::changed, this, [this]() {
        handle_slider_change(1);
    });
    connect(level, &gradient_slider::changed, this, [this]() {
        handle_slider_change(2);
    });

    auto* layout = make_slider_layout({hue, sat, level});
    setLayout(layout);
}

inline void hsl_selector::handle_color_change() {
    auto hsl_c = hsl(selection_->rgb());

    if(hsl_c.is_gray()) {
        hsl_c.set_h(hsl_color.h());
    }

    hsl_color = hsl_c;

    hue->set_value(hsl_color.h()*hue_slider_range);
    sat->set_value(hsl_color.s()*sat_range);
    level->set_value(hsl_color.l()*lvl_range);

    rethink_gradients(-1);
}

inline void hsl_selector::handle_slider_change(int which_slider) {
    const auto h = hue->value();
    const auto s = sat->value();
    const auto l = level->value();
    hsl_color = hsl(h*hue_color_irange, s*sat_irange, l*lvl_irange);
    selection_->set_rgb(hsl_color.to_rgb(), color_id);

    rethink_gradients(which_slider);
}

inline void hsl_selector::rethink_gradients(int unchanged_slider) {
    const auto c = hsl_color;

    if(unchanged_slider != 0) {
        hue_gradient->set(c.s(), c.l(), hue->gradient()->size());
        hue->set_gradient(hue_gradient);
    }

    if(unchanged_slider != 1) {
        sat_gradient->set(c, sat->gradient()->size());
        sat->set_gradient(sat_gradient);
    }

    if(unchanged_slider != 2) {
        level_gradient->set(c, level->gradient()->size());
        level->set_gradient(level_gradient);
    }
}



} // namespace cosel


#endif // cosel_qt_select_group_hsl_h
