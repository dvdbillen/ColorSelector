// SPDX-License-Identifier: MIT
// Copyright (c) 2026 David Billen

// hsv_selector for both hsv and hsv2

#ifndef cosel_qt_select_group_hsv_h
#define cosel_qt_select_group_hsv_h

#include "cosel_qt_select_group_base.h"

namespace cosel {


////////////////////////////////////////////////////////////////////////////////////////////////////

template<typename HSV_T>
class hsv_selector : public color_selector {
public:
    explicit hsv_selector(selector_type_t type, const selected_color_t& color,
                          const gradient_slider::style_options_t& options_) :
        color_selector(type, color, options_) {
        initialize();
    }

private:
    static_assert(std::is_same_v<HSV_T, hsv> || std::is_same_v<HSV_T, hsv2>,
                  "T must be either hsv or hsv2");

    // Hue gradient is an hsx_hue_gradient_t<HSV_T>

    using hue_gradient_t = hsx_hue_gradient_t<HSV_T>;

    // Sat gradient is a linear gradient

    class sat_gradient_t : public linear_gradient_t<HSV_T> {
    public:
        explicit sat_gradient_t(const HSV_T& c_, size_t n) :
            linear_gradient_t<HSV_T>(c_, n, [](const HSV_T& c, double pos) {
            return c.with_s(pos).to_rgb();
        }) {}

    };

    // V gradient is an l_gradient, (illustrates range using LAB L luminance)

    using hsvl_gradient_t = l_gradient_t;

    constexpr static double hue_slider_range = 359;
    constexpr static double hue_color_range = 360;
    constexpr static double sat_range = 100;
    constexpr static double lvl_range = 100;

    constexpr static double hue_color_irange = 1/hue_color_range;
    constexpr static double sat_irange = 1/sat_range;
    constexpr static double lvl_irange = 1/lvl_range;

    HSV_T hsv_color;

    gradient_slider* hue = nullptr;
    gradient_slider* sat = nullptr;
    gradient_slider* level = nullptr;

    std::shared_ptr<hue_gradient_t> hue_gradient;
    std::shared_ptr<sat_gradient_t> sat_gradient;
    std::shared_ptr<hsvl_gradient_t> level_gradient;

    void initialize();
    void handle_color_change() override;
    void handle_slider_change(int which_slider);
    void rethink_gradients(int unchanged_slider);
};

template<typename HSV_T>
inline void hsv_selector<HSV_T>::initialize() {
    hsv_color = HSV_T(selection_->rgb());

    auto* hgrad = new hue_gradient_t(1, 1, 50);
    hue_gradient = std::shared_ptr<hue_gradient_t>(hgrad);
    auto* sgrad = new sat_gradient_t(HSV_T(1, 1, 1), 50);
    sat_gradient = std::shared_ptr<sat_gradient_t>(sgrad);
    auto* lgrad = new hsvl_gradient_t(HSV_T(1, 0, 1).to_rgb(), 50);
    level_gradient = std::shared_ptr<hsvl_gradient_t>(lgrad);

    const auto& info = get_selector_info(selector_type_t::hsv);
    const auto& names = info.component_names;

    hue = new gradient_slider(names[0], 0, hue_slider_range, options, hsv_color.h()*hue_color_range);
    hue->set_gradient(hue_gradient);

    sat = new gradient_slider(names[1], 0, sat_range, options, hsv_color.s()*sat_range);
    sat->set_gradient(sat_gradient);

    level = new gradient_slider(names[2], 0, lvl_range, options, hsv_color.v()*lvl_range);
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

template<typename HSV_T>
inline void hsv_selector<HSV_T>::handle_color_change() {
    auto hsv_c = HSV_T(selection_->rgb());

    if(hsv_c.is_gray()) {
        hsv_c.set_h(hsv_color.h());
    }

    hsv_color = hsv_c;

    hsv_color = HSV_T(selection_->rgb());

    hue->set_value(hsv_color.h()*hue_color_range);
    sat->set_value(hsv_color.s()*sat_range);
    level->set_value(hsv_color.v()*lvl_range);

    rethink_gradients(-1);
}

template<typename HSV_T>
inline void hsv_selector<HSV_T>::handle_slider_change(int which_slider) {
    const auto h = hue->value();
    const auto s = sat->value();
    const auto l = level->value();
    hsv_color = HSV_T(h*hue_color_irange, s*sat_irange, l*lvl_irange);
    selection_->set_rgb(hsv_color.to_rgb(), color_id);

    rethink_gradients(which_slider);
}

template<typename HSV_T>
inline void hsv_selector<HSV_T>::rethink_gradients(int unchanged_slider) {
    const auto c = hsv_color;

    if(unchanged_slider != 0) {
        hue_gradient->set(c.s(), c.v(), hue->gradient()->size());
        hue->set_gradient(hue_gradient);
    }

    if(unchanged_slider != 1) {
        sat_gradient->set(c, sat->gradient()->size());
        sat->set_gradient(sat_gradient);
    }

    if(unchanged_slider != 2) {
        level_gradient->set_color(HSV_T(c.h(), c.s(), 1).to_rgb());
        level->set_gradient(level_gradient);
    }
}


} // namespace cosel


#endif // cosel_qt_select_group_hsv_h
