// SPDX-License-Identifier: MIT
// Copyright (c) 2026 David Billen

// Base object for color selectors.

#ifndef cosel_qt_select_group_base_h
#define cosel_qt_select_group_base_h
#include <memory>

#include <QBoxLayout>
#include <QColor>
#include <QDebug>
#include <QShowEvent>
#include <QWidget>

#include "cosel_gradient.h"
#include "cosel_model.h"
#include "cosel_model_hsx.h"
#include "cosel_qt_gradient_slider.h"
#include "cosel_qt_selected_color.h"
#include "cosel_qt_slider_model_info.h"

namespace cosel {

class color_selector : public QWidget {
public:
    ~color_selector() override =default;
    explicit color_selector(selector_type_t type,
                            const selected_color_t& color,
                            const gradient_slider::style_options_t& options_);

    selector_type_t type() const {
        return type_;
    }
protected:
    selector_type_t type_ = selector_type_t::rgb;
    gradient_slider::style_options_t options;
    selected_color_t selection_;
    unsigned color_id = 0;

    virtual void handle_color_change() = 0;

    // Make the vbox with the sliders and other base processing. Also sets the slider pointers.
    QVBoxLayout* make_slider_layout(const std::vector<gradient_slider*>& sliders);

    QSize sizeHint() const override {
        return QSize(konst::default_selector_width(),
                     3*(options.height + konst::selector_height_padding()));
    }
private:
    std::vector<gradient_slider*> slider_pointers;
};

inline color_selector::color_selector(selector_type_t type,
                                      const selected_color_t& color,
                                      const gradient_slider::style_options_t& options_) :
    type_(type),
    options(options_),
    selection_(color),
    color_id(color->assign_id()) {

    // Handle color change signal and dispatch to derived class if it did not originate the change
    connect(selection_.get(), &selected_color::changed, this, [this](unsigned id) {
        if(isVisible()) {
            if(id != color_id) {
                handle_color_change();
            }
        }
    });

    // It's only useful to stretch horiz
    setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
}

inline QVBoxLayout* color_selector::make_slider_layout(
    const std::vector<gradient_slider*>& sliders) {

    slider_pointers = sliders;

    auto* layout = new QVBoxLayout();

    for(unsigned slider = 0; slider < slider_pointers.size(); ++slider) {

        // Add to layout
        if(slider != 0) {
            layout->addSpacing(konst::selector_height_padding());
        }

        layout->addWidget(slider_pointers[slider]);
    }

    return layout;
}



} // namespace cosel


#endif // cosel_qt_select_group_base_h
