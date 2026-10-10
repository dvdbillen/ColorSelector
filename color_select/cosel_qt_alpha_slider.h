// SPDX-License-Identifier: MIT
// Copyright (c) 2026 David Billen

// Alpha slider. Two widgets are defined: A basic slider which renders a gradient for selecting
// alpha (alpha_slider) and a widget that derives from an alpha_slider and works with the shared
// color selection (alpha_widget).


#ifndef cosel_qt_alpha_slider_h
#define cosel_qt_alpha_slider_h

#include <array>

#include <QBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <QTimer>
#include <QWidget>


#include "cosel_geometry.h"
#include "cosel_gradient.h"
#include "cosel_qt_alphapixmap.h"
#include "cosel_qt_custom_slider.h"
#include "cosel_qt_gradient_slider.h"
#include "cosel_qt_gradpixmap.h"
#include "cosel_qt_render_helper.h"
#include "cosel_qt_selected_color.h"
#include "cosel_qt_slider_label.h"
#include "cosel_qt_swatch.h"

namespace cosel {

class alpha_slider : public QWidget {
    Q_OBJECT;

public:
    explicit alpha_slider(gradient_slider::style_options_t options_, double range, double current_,
                          QWidget* parent = nullptr);

    void set_color(const rgb& c);
    const rgb& color() const {
        return color_;
    }

    void set_value(double value) {
        set_value(value, true);
    }
    double value() const {
        return current;
    }

    double range() const {
        return range_;
    }

private:
    QColor bg_color;
    QColor fg_color;
    QString label_ = tr("Alpha");
    QPixmap pointer_pixmap;
    double pointer_dpr = 0;


    constexpr static int minimum_width = 64;

    // Slider options
    gradient_slider::style_options_t options;
    // Display range_
    double range_ = 100;
    // Current color
    rgb color_;
    // Current value of slider
    double current = 0;
    // See the custom slider class. That class is designed to be used as inherited or compositional.
    // Here we choose the latter which requires a lambda to be assigned for the rendering.
    custom_slider* slider = nullptr;
    // Label positioned to the left of the slider
    slider_label_display* label = nullptr;
    // The editable display of the slider value position to the right of the slider
    slider_label_edit* edit = nullptr;

    // Legend is rendered to this pixmap
    QPixmap alpha_pixmap;

    double slider_to_value(double n) const {
        return clamp(n)*range_;
    }

    double value_to_slider(double v) const {
        return clamp(v/range_);
    }

    // This returns the pixmap size which is not straightforward because it excludes the size of the
    // pointer (knob) which can overhang the edges, and also considers that we render the pixmaps
    // with 1:1 pixel resolution regardless of display scaling.
    QSize calc_pixmap_size() const {
        assert(slider != nullptr);
        const auto w = slider->width() - slider->knob_size().width();
        const auto h = slider->knob_size().height();
        const auto pixel_size = QSize(w, h);
        return(rescaled(this, pixel_size));
    }

    // Function to render the contents of the custom_slider.
    void slider_painter(QPainter* p, const QRect* r, const QRect* kr, slider_state state);

    // Private version of set_value that provides option to sync the slider.
    void set_value(double value, bool sync) {
        current = std::clamp(value, 0.0, range_);

        if(edit != nullptr) {
            edit->set(current);
        }

        if(sync) {
            const auto n = value_to_slider(current);
            slider->set(n);
        }
    }

    void resizeEvent([[maybe_unused]] QResizeEvent* ev) override;
signals:
    void changed();
};

inline alpha_slider::alpha_slider(
    gradient_slider::style_options_t options_, double range, double current_, QWidget* parent) :
    QWidget(parent),
    options(options_.with_height(options_.height/2)),
    range_(range),
    current(current_) {

    std::tie(bg_color, fg_color) = get_bg_fg_colors(this);

    slider = new custom_slider(0, options.height, gradient_slider_knob_width);

    // The incrementer is used by the keyboard control to decide how much distance to travel for
    // a right/left arrow press.
    slider->set_incrementer([this](double n, bool dec) {
        auto v = std::floor(slider_to_value(n));
        v += dec? -2: 2;
        v = std::clamp(v, 0.0, range_);
        return value_to_slider(v);
    });

    // Set the lambda for rendering the custom slider to our slider_paint() function.
    slider->set_painter([this](QPainter* p, const QRect* r, const QRect* kr, slider_state state) {
        slider_painter(p, r, kr, state);
    });

    // Handle custom slider changes
    connect(slider, &custom_slider::changed, this, [this]() {
        const auto n = slider->value();
        const auto value = slider_to_value(n);
        set_value(value, false);
        emit changed();
    });

    // Create label and editbox if the options allow it
    if(options_.info) {
        label = new slider_label_display(label_);
        label->setFixedSize(gradient_slider_knob_width*2, gradient_slider_knob_width);

        edit = new slider_label_edit(0, range_, 2);
        edit->setFixedSize(gradient_slider_knob_width*2, options.height);

        connect(edit, &slider_label_edit::edited, this, [this](double n) {
            set_value(n);
            emit changed();
        });

        label->setFocusPolicy(Qt::NoFocus);
        edit->setFocusPolicy(Qt::StrongFocus);
    }

    slider->setFocusPolicy(Qt::StrongFocus);

    // Handle the layout

    auto* layout = new QHBoxLayout();
    layout->setContentsMargins(0, 0, 0, 0);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    setMinimumWidth(minimum_width);
    setMinimumHeight(std::max<int>(options.height, gradient_slider_knob_width));

    if(label != nullptr) {
        layout->addWidget(label);
    }

    layout->addWidget(slider);

    if(edit != nullptr) {
        layout->addWidget(edit);
    }

    setLayout(layout);

    // Defer setting the slider to the value
    defer(25, [this]() {
        slider->setFocus();
        set_value(current);
        emit changed();
    });
}

inline void alpha_slider::set_color(const rgb& c) {
    color_ = c;

    if(slider == nullptr) {
        return;
    }

    // Regenerate the pixelmap and update
    const auto pixmap_size = calc_pixmap_size();

    const auto image = make_alpha_pixmap(color_, pixmap_size.width(), pixmap_size.height(),
                                     options.rounding);
    alpha_pixmap = QPixmap::fromImage(image);
    alpha_pixmap.setDevicePixelRatio(devicePixelRatioF());

    slider->update();
}

inline void alpha_slider::resizeEvent([[maybe_unused]] QResizeEvent* ev) {
    if(slider == nullptr) {
        return;
    }

    const auto pixmap_size = calc_pixmap_size();

    if(pixmap_size.width() < 4 || pixmap_size.height() < 4) {
        return;
    }

    const auto image = make_alpha_pixmap(color_, pixmap_size.width(), pixmap_size.height(),
                                     options.rounding);
    alpha_pixmap = QPixmap::fromImage(image);
    alpha_pixmap.setDevicePixelRatio(devicePixelRatioF());
    slider->update();
}

inline void alpha_slider::slider_painter(QPainter* p, [[maybe_unused]] const QRect* r,
        const QRect* kr, slider_state state) {

    if(alpha_pixmap.isNull()) {
        return;
    }

    // Render alpha legend
    QRect real_r(QPoint(slider->knob_size().width()/2, 0),
                 unscaled(this, QSize(alpha_pixmap.size())));


    p->drawPixmap(real_r.x(), real_r.y(), alpha_pixmap);

    // Outline it
    const auto outline_rect = QRectF(real_r).adjusted(.5, .5, -.5, -.5);

    p->setRenderHint(QPainter::Antialiasing);
    QPen outline_pen(with_alpha(fg_color, 48));
    outline_pen.setCosmetic(true);
    p->setPen(outline_pen);
    p->setBrush(Qt::NoBrush);

    if(options.rounding > 0) {
        // Round to integer to match rounding in generated legend
        const auto round_linesf = options.rounding*(alpha_pixmap.height());
        const auto round_lines = std::lround(unscaled(this, round_linesf)*.5);
        const auto radius = double(round_lines);
        p->drawRoundedRect(outline_rect, radius, radius);
    } else {
        p->drawRect(outline_rect);
    }

    // Render pointer if enabled
    if(state == slider_state::disabled) {
        return;
    }

    if(pointer_pixmap.isNull() || devicePixelRatioF() != pointer_dpr) {
        pointer_pixmap = render_pointer(this, kr, options.pointer_top);
        pointer_dpr = pointer_pixmap.devicePixelRatioF();
    }

    const auto x = kr->x();
    const auto y = options.pointer_top? int(0): int((real_r.height() - kr->width()*.5) + .5);
    p->drawPixmap(x, y, pointer_pixmap);
}


////////////////////////////////////////////////////////////////////////////////////////////////////

class alpha_widget : public alpha_slider {
public:
    explicit alpha_widget(const selected_color_t& selection,
                          gradient_slider::style_options_t options_, double range_, double current_,
                          QWidget* parent = nullptr);

private:
    selected_color_t selection_;
    unsigned id = 0;
};

inline alpha_widget::alpha_widget(const selected_color_t& selection,
                                  gradient_slider::style_options_t options_,
                                  double range_, double current_, QWidget* parent) :
    alpha_slider(options_, range_, current_, parent),
    selection_(selection),
    id(selection->assign_id()) {

    // Handle our own changed signal
    connect(this, &alpha_slider::changed, this, [this]() {
        selection_->set_alpha(value()/range(), id);
    });

    // Respond to color changes
    connect(selection_.get(), &selected_color::changed, this, [this](unsigned id_) {
        // Currently we don't change the color, but check id in the name of good practice
        if(id_ != id) {
            set_color(selection_->rgb());
        }
    });

    // Respond to alpha changes
    connect(selection_.get(), &selected_color::alpha_changed, this, [this](unsigned id_) {
        if(id_ != id) {
            set_value(selection_->alpha()*range());
        }
    });

}

} // namespace cosel


#endif // cosel_qt_alpha_slider_h
