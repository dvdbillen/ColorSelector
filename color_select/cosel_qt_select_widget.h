// SPDX-License-Identifier: MIT
// Copyright (c) 2026 David Billen

// Parent widget that supports various configurations of color selection controls

// This is the contents of the Qt mock dialog. It can also be used as you would any other widget.

#ifndef cosel_qt_select_widget_h
#define cosel_qt_select_widget_h

#include <memory>

#include <QBoxLayout>
#include <QColor>
#include <QDebug>
#include <QPushButton>
#include <QSize>
#include <QStackedWidget>
#include <QTabBar>
#include <QTabWidget>
#include <QWidget>

#include "cosel_qt_2d_widget.h"
#include "cosel_qt_alpha_slider.h"
#include "cosel_qt_app_defs.h"
#include "cosel_qt_html_widget.h"
#include "cosel_qt_palette.h"
#include "cosel_qt_ring.h"
#include "cosel_qt_screen_picker.h"
#include "cosel_qt_selected_color.h"
#include "cosel_qt_select_group_base.h"
#include "cosel_qt_select_group_hsl.h"
#include "cosel_qt_select_group_hsv.h"
#include "cosel_qt_select_group_hsl2.h"
#include "cosel_qt_select_group_rgb.h"

namespace cosel {


class select_widget : public QWidget {
    Q_OBJECT;

    constexpr static size_t num_types = num_selector_types;
public:
    struct options_t {
        // Slider style defined in cosel_qt_gradient_slider.h
        gradient_slider::style_options_t slider_style;

        // Enable disable slider types indexed by selector_type_t. If you enable only one slider
        // the tab control won't appear. There are helper functions in this struct so you don't
        // have to index this array using enums.
        std::array<bool, num_types> slider_enables = {true, true, true, false, false};

        // Show an alpha slider. The main swatch will reflect the alpha level.
        bool use_alpha = false;

        // Show a screen color picker.
        bool use_screen_picker = true;

        // Show the 2D color widgets.
        bool two_d_widget = true;

        // Resolution of the 2D HSV selector in pixel w/h (square). Clamped 1:32. Set to 1 for
        // non-segmented selector.
        int two_d_resolution = konst::default_2d_resolution();

        // If resolution >= 4 this outlines each color with the bg color. NOTE: When using display
        // scaling on Windows this may create artifacts if the resolution is not a multiple of 4.
        bool two_d_bordered = konst::default_2d_bordered();

        options_t() = default;

        // Helpers to simplify setting slider enables
        void enable_rgb(bool onoff) {
            slider_enables[size_t(selector_type_t::rgb)] = onoff;
        }
        void enable_hsv(bool onoff) {
            slider_enables[size_t(selector_type_t::hsv)] = onoff;
        }
        void enable_hsl(bool onoff) {
            slider_enables[size_t(selector_type_t::hsl)] = onoff;
        }
        void enable_hsv2(bool onoff) {
            slider_enables[size_t(selector_type_t::hsv2)] = onoff;
        }
        void enable_hsl2(bool onoff) {
            slider_enables[size_t(selector_type_t::hsl2)] = onoff;
        }
    };

    // This should only be set by the select_widget. It can be obtained before destroying the class
    // and passed back to the ctor so that the user opens a dialog with the same active slider
    // group, (etc), as when they closed it.

    struct state_t {
        // If the slider_tab is < 0 then the specified inital_type in the select_widget constructor
        // is used
        int slider_tab = -1;
        // If the twod_tab is < 0 then the specified initial_twod in the select widget constructor
        // is used
        int twod_tab = -1;
        // The only valid initialization of this is from the palette object
        QString custom_colors;

        state_t() = default;
    };

    select_widget(options_t options, selector_type_t initial_type, twod_type_t initial_twod,
                  selected_color_t selection,
                  state_t state,
                  QWidget* parent = nullptr);

    const state_t& state() const {
        return state_;
    }

signals:
    // Emitted when state data (from select_widget::state_t) changes. You can save the state in
    // response to this signal if there's not a clean opportunity to read it before the class
    // instance goes out of scope.
    void state_changed(const cosel::select_widget::state_t& state);

private:
    options_t options_;
    selected_color_t selection_;
    unsigned slider_selection_id = 0;
    unsigned swatch_selection_id = 0;
    state_t state_;

    QTabWidget* stacked_sliders = nullptr;
    color_swatch* swatch_widget = nullptr;
    QTabWidget* stacked_2d = nullptr;
    alpha_widget* alpha = nullptr;
    palette_widget* palette = nullptr;
    QPushButton* screen_picker = nullptr;
    html_widget* html_widget_inst = nullptr;

    [[nodiscard]] QTabWidget* make_2d(twod_type_t type);
    [[nodiscard]] QTabWidget* make_sliders(selector_type_t initial_type);
    [[nodiscard]] color_swatch* make_swatch();
    [[nodiscard]] html_widget* make_html_widget();
    [[nodiscard]] alpha_widget* make_alpha_widget();
    [[nodiscard]] palette_widget* make_palette_widget();
    [[nodiscard]] QPushButton* make_screen_picker();

    [[nodiscard]] QLayout* make_layout();
    [[nodiscard]] QLayout* make_top_layout();
    // Can return nullptr if there isn't one!
    [[nodiscard]] QLayout* make_bottom_layout();

    // Used by make_sliders to make an individual group
    [[nodiscard]] color_selector* make_type(selector_type_t type, QWidget* parent);
};


////////////////////////////////////////////////////////////////////////////////////////////////////
// Create main widget

// If the state specifies a slider_tab >= 0 then the initial_type is ignored. If the initial_type
// is not enabled in the options (and the slider_tab < 0) then the initial visible type is
// undefined.
//
// If the state specified a twod_tab >= 0 then the initial_twod is ignored.
inline select_widget::select_widget(options_t options,
                                    selector_type_t initial_type, twod_type_t initial_twod,
                                    selected_color_t selection,
                                    state_t state, QWidget* parent) :
    QWidget(parent),
    options_(options),
    selection_(std::move(selection)),
    state_(std::move(state)) {

    slider_selection_id = selection_->assign_id();
    swatch_selection_id = selection_->assign_id();

    stacked_2d = make_2d(initial_twod);
    stacked_sliders = make_sliders(initial_type);
    swatch_widget = make_swatch();
    palette = make_palette_widget();
    alpha = make_alpha_widget();
    screen_picker = make_screen_picker();
    html_widget_inst = make_html_widget();

    setLayout(make_layout());
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Create slider groups
inline QTabWidget* select_widget::make_sliders(selector_type_t initial_type) {
    // If there's not a valid initial type then set to unspecified (the first one enabled will be
    // used).
    auto init_type = size_t(initial_type);

    if(init_type < num_types && !options_.slider_enables[init_type]) {
        init_type = num_types;
    }

    auto* sliders = new QTabWidget(this);
    sliders->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);

    int starting_index = 0;

    for(unsigned i = 0; i < num_types; ++i) {
        if(!options_.slider_enables[i]) {
            continue;
        }

        if(i == init_type) {
            starting_index = sliders->count();
        }

        const auto& info = get_selector_info(i);

        color_selector* selector_widget = make_type(selector_type_t(i), this);
        const auto index = sliders->addTab(selector_widget, info.name);
        selector_widget->setProperty("tabindex", QVariant::fromValue(index));
    }

    if(state_.slider_tab >= 0 && starting_index < sliders->count()) {
        starting_index = state_.slider_tab;
    } else {
        starting_index = std::min(starting_index, sliders->count() - 1);
    }

    sliders->setCurrentIndex(starting_index);

    connect(sliders, &QTabWidget::currentChanged, this, [this]([[maybe_unused]] int index) {
        // When the tab changes, simulate a color change so the newly activated widget will update
        emit selection_->changed(slider_selection_id);
        // Record and broadcast the state change
        state_.slider_tab = index;
        emit state_changed(state_);
    });

    sliders->setTabBarAutoHide(true);

    // emit a color change so the default tab will sync (a bit delayed)
    defer(50, [this]() {
        emit selection_->changed(slider_selection_id);
    });

    return sliders;
}

// Create one slider group by type
inline color_selector* select_widget::make_type(selector_type_t type, QWidget* parent) {
    color_selector* selector = nullptr;

    switch(type) {
        case selector_type_t::rgb: {
            selector = new rgb_selector(type, selection_, options_.slider_style);
            break;
        }

        case selector_type_t::hsl: {
            selector = new hsl_selector(type, selection_, options_.slider_style);
            break;
        }

        case selector_type_t::hsv: {
            selector = new hsv_selector<hsv>(type, selection_, options_.slider_style);
            break;
        }

        case selector_type_t::hsl2: {
            selector = new hsl2_selector(type, selection_, options_.slider_style);
            break;
        }

        case selector_type_t::hsv2: {
            selector = new hsv_selector<hsv2>(type, selection_, options_.slider_style);
            break;
        }
    }

    selector->setParent(parent);

    return selector;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// Create optional alpha widget
inline alpha_widget* select_widget::make_alpha_widget() {
    if(!options_.use_alpha) {
        return nullptr;
    }

    return new alpha_widget(selection_, options_.slider_style, 255, 255, this);
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// Create html widget
inline html_widget* select_widget::make_html_widget() {
    return new html_widget(selection_, this);
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// Create button that invokes screen picker
inline QPushButton* select_widget::make_screen_picker() {
    if(!options_.use_screen_picker) {
        return nullptr;
    }

    auto* screen_picker = new QPushButton(tr("Pick from Screen..."), this);
    connect(screen_picker, &QPushButton::clicked, this, [this]() {
        const auto qt_c = pick_screen_color(this);

        if(qt_c.isValid()) {
            selection_->set_qcolor(qt_c, 0);
        }
    });

    return screen_picker;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// Create palette widget (standard and custom colors)
inline palette_widget* select_widget::make_palette_widget() {
    auto* pal = new palette_widget(selection_, options_.slider_style.rounding);
    pal->setToolTip(tr("Drag and drop"));

    if(!state_.custom_colors.isEmpty()) {
        pal->set_custom_color_string(state_.custom_colors);
    }

    connect(pal, &palette_widget::custom_palette_changed, this, [this, pal]() {
        state_.custom_colors = pal->custom_color_string();
        emit state_changed(state_);
    });

    return pal;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// Create main swatch that shows current color selection
inline color_swatch* select_widget::make_swatch() {
    auto* swatch = new cosel::color_swatch(cosel::xyz(), options_.slider_style.rounding);
    swatch->setFixedSize(konst::swatch_size());

    swatch->setToolTip(tr("Drag and drop"));

    // When a color is dropped onto the main color swatch it should make a selection
    connect(swatch, &color_swatch::dropped, this, [this, swatch]() {
        const auto c = swatch->get();
        selection_->set_rgb(c, swatch_selection_id);
    });

    // Sync the swatch to color changes
    const auto* the_color_ptr = selection_.get();
    connect(the_color_ptr, &cosel::selected_color::changed, this,
    [this, swatch](unsigned id) {
        if(id != swatch_selection_id) {
            swatch->set(selection_->xyz());
        }
    });

    // Sync the swatch to alpha changes
    connect(the_color_ptr, &cosel::selected_color::alpha_changed, this,
    [this, swatch](unsigned) {
        swatch->set_alpha(selection_->alpha());
    });

    return swatch;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// Create tabbed 2d widget(s)
inline QTabWidget* select_widget::make_2d(twod_type_t type) {
    if(!options_.two_d_widget) {
        return nullptr;
    }

    // Create HSV 2d widget
    auto* widget_2d = new twod_selector(selection_, options_.slider_style.rounding,
            options_.two_d_resolution, options_.two_d_bordered);

    // Create HSV ring widget
    auto* ring_widget = new selection_ring(selection_, this);

    // Add to stack
    auto* stack = new QTabWidget(this);

    auto info = get_twod_info(twod_type_t::grid);
    stack->addTab(widget_2d, info.name);

    info = get_twod_info(twod_type_t::ring);
    stack->addTab(ring_widget, info.name);

    stack->setFixedSize(konst::default_2d_size());
    stack->setTabPosition(QTabWidget::South);

    auto start_index = state_.twod_tab;

    if(start_index < 0) {
        if(type == twod_type_t::grid) {
            start_index = 0;
        } else {
            start_index = 1;
        }
    }

    stack->setCurrentIndex(start_index);

    connect(stack, &QTabWidget::currentChanged, this, [this]([[maybe_unused]] int index) {
        // Record and broadcast the state change
        state_.twod_tab = index;
        emit state_changed(state_);
    });


    return stack;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// Create layout
inline QLayout* select_widget::make_layout() {
    auto* main_layout = new QVBoxLayout();
    main_layout->addStretch();
    main_layout->addLayout(make_top_layout());
    main_layout->addSpacing(4);
    main_layout->addWidget(stacked_sliders);
    auto* bottom_layout = make_bottom_layout();

    if(bottom_layout != nullptr) {
        main_layout->addLayout(bottom_layout);
    }

    return main_layout;
}

// Top layout contains the 2d widgets, palette, and swatch
inline QLayout* select_widget::make_top_layout() {
    // The extra select options and swatch are in a horiz layout
    auto* extras_layout = new QHBoxLayout();
    extras_layout->setContentsMargins(0, 0, 0, 0);
    extras_layout->addWidget(html_widget_inst);

    if(options_.use_screen_picker && screen_picker != nullptr) {
        extras_layout->addSpacing(8);
        extras_layout->addWidget(screen_picker);
    }

    extras_layout->addStretch(2);
    extras_layout->addWidget(swatch_widget, 1);


    // Use a vert layout to fit the swatch beneath the palette
    auto* pal_and_swatch_layout = new QVBoxLayout();
    pal_and_swatch_layout->setContentsMargins(0, 0, 0, 0);
    pal_and_swatch_layout->addWidget(palette);
    pal_and_swatch_layout->addLayout(extras_layout);

    // Finish the layout with the palette to the right of the 2d widgets
    auto* top_layout = new QHBoxLayout();
    top_layout->setContentsMargins(0, 0, 0, 0);

    if(options_.two_d_widget) {
        top_layout->addWidget(stacked_2d, 0, Qt::AlignTop);
        top_layout->addStretch();
    }

    top_layout->addLayout(pal_and_swatch_layout);

    return top_layout;
}

// The bottom layout the alpha widget if used else it is nullptr
inline QLayout* select_widget::make_bottom_layout() {
    if(!options_.use_alpha) {
        // Currently a bottom layout only used for alpha slider
        return nullptr;
    }

    auto* bottom_layout = new QHBoxLayout();
    bottom_layout->setContentsMargins(0, 0, 0, 0);
    bottom_layout->addStretch(1);
    bottom_layout->addWidget(alpha, 2, Qt::AlignVCenter);
    // TODO: This works out with the test style, but needs to always align if possible
    bottom_layout->addSpacing(10);

    return bottom_layout;
}



} // namespace cosel


#endif // cosel_qt_select_widget_h
