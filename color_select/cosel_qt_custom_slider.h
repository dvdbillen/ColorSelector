// SPDX-License-Identifier: MIT
// Copyright (c) 2026 David Billen

// Custom rendered slider control derived straight from QWidget This class was ported from a
// repository to implement our sliders with fully custom rendering. It is designed to support
// either inheritance or compositional programming style.

#ifndef cosel_qt_custom_slider_h
#define cosel_qt_custom_slider_h

#include <functional>

#include <QColor>
#include <QBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QPoint>
#include <QRect>
#include <QString>
#include <QWidget>

#include "cosel_math.h"

namespace cosel {

enum class slider_state : std::int8_t {
    normal,
    disabled,
    knob_hovered, // control hovered inside knob
    rect_hovered, // control hovered, but not inside knob
    pressed
};

// Arguments are a prepared painter, the full rect(), the knob rect, and slider state.
using slider_painter_t = std::function<void(QPainter*, const QRect*, const QRect*, slider_state)>;
// Arguments are the current value and a flag indicating decrement (else increment)
using slider_inc_t = std::function<double(double n, bool decrement)>;

class custom_slider : public QWidget {
    Q_OBJECT;

public:
    explicit custom_slider(double initial_pos, int height, int knob_width_, QWidget* parent = nullptr);

    double value() const {
        return local_pos;
    }

    void set(double n);

    QSize knob_size() const {
        return QSize{knob_width, height()};
    }

    QRect legend_rect() const {
        return {QPoint(knob_width/2, 0), QSize(width() - knob_width, height())};
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // For compositional use set these two functions. Alternatively you can derive and override
    // the render and increment functions.

    // This sets a function to render the slider. If this function has been set then the render
    // function is not used.
    void set_painter(slider_painter_t p) {
        painter_func = std::move(p);
    }

    // This sets a function to determine how much to move the slider in a single increment,
    // normally in response to a left/right key press. The function accepts a current value and a
    // flag. The flag is true if the function should decrement. If this function has been set then
    // the render function is not used.
    void set_incrementer(slider_inc_t i) {
        incrementer_func = std::move(i);
    }

    // You can set the a painter function instead of overriding this. This will not be called if a
    // painter has been set.
    virtual void render(const QRect& knob, slider_state state);

    // You can set the incrementer function instead of overriding this. This will not be called if
    // an incrementer has been set.
    virtual double increment(double n, bool decrement) const {
        const auto amount = decrement? -.1: .1;
        return n + amount;
    }

protected:
    // Return the X range of the slider in pixels
    [[nodiscard]] int pixel_range() const {
        return width() - knob_width;
    }

    // Convert a pixel position to a normal (0-1)
    [[nodiscard]] double pixel_to_pos(int pixel) const {
        const auto n = pixel_to_pos_scale*pixel;
        return clamp(n);
    }

    // Convert a normal (0-1) position to a pixel position
    [[nodiscard]] int pos_to_pixel(double pos) const {
        const auto n = pos_to_pixel_scale*clamp(pos);
        return static_cast<int>(std::round(n));
    }

    // Return the rectangle for the knob in its current position
    [[nodiscard]] QRect knob_rect() const {
        const auto pos = local_pos;
        return {QPoint(pos_to_pixel(pos), 0), knob_size()};
    }

    // Recalculate the coefficients used to convert between pixel position and normal position
    void calc_scales() {
        const auto px_range = pixel_range();

        if(px_range > 0) {
            pixel_to_pos_scale = 1.0/px_range;
            pos_to_pixel_scale = double(px_range);
        }
    }

    void paintEvent([[maybe_unused]] QPaintEvent* pe) override;
    void resizeEvent(QResizeEvent* event) override;

private:
    double pixel_to_pos_scale = 0;
    double pos_to_pixel_scale = 0;

    bool hovering = false;
    bool pressed = false;
    double local_pos = 0;
    int knob_width = 0;
    QPoint last_move;
    QPoint move_offset;

    slider_painter_t painter_func;
    slider_inc_t incrementer_func;

    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent*) override;
    void focusOutEvent(QFocusEvent*) override;
    void keyPressEvent(QKeyEvent* event) override;

signals:
    void changed();
};

inline void custom_slider::keyPressEvent(QKeyEvent* event) {
    auto pos = local_pos;

    switch(event->key()) {
        case Qt::Key_Home:
            pos = 0;
            break;

        case Qt::Key_End:
            pos = 1;
            break;

        case Qt::Key_Left:
            if(incrementer_func) {
                pos = incrementer_func(pos, true);
            } else {
                pos = increment(pos, true);
            }

            break;

        case Qt::Key_Right:
            if(incrementer_func) {
                pos = incrementer_func(pos, false);
            } else {
                pos = increment(pos, false);
            }

            break;


        default:
            QWidget::keyPressEvent(event);
            return;
    }

    this->set(pos);
    emit changed();
}

inline void custom_slider::paintEvent([[maybe_unused]] QPaintEvent* pe) {
    // Determine slider state
    slider_state s = slider_state::normal;

    if(!isEnabled()) {
        s = slider_state::disabled;
    } else if(pressed) {
        s = slider_state::pressed;
    } else if(hovering) {
        if(knob_rect().contains(last_move)) {
            s = slider_state::knob_hovered;
        } else {
            s = slider_state::rect_hovered;
        }
    }

    // Render using function or class member
    const auto kr = knob_rect();

    if(kr.isValid() && legend_rect().isValid()) {
        if(painter_func) {
            // Call the user specified function
            QPainter p(this);
            const auto r(rect());
            painter_func(&p, &r, &kr, s);
        } else {
            // Call the class member
            render(kr, s);
        }
    }
}

inline void custom_slider::resizeEvent(QResizeEvent* event) {
    calc_scales();
    QWidget::resizeEvent(event);
}

inline custom_slider::custom_slider(double initial_pos, int height, int knob_width_,
                                    QWidget* parent) :
    QWidget(parent),
    local_pos(initial_pos),
    knob_width(std::max(1, knob_width_)) {

    setFocusPolicy(Qt::StrongFocus);
    setContentsMargins(0, 0, 0, 0);
    setFixedHeight(height);
    resize(height*2, height);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setMouseTracking(true);

    calc_scales();
}

inline void custom_slider::set(double n) {
    n = clamp(n);

    if(n != local_pos) {
        local_pos = n;

        update();
    }
}

inline void custom_slider::render(const QRect& knob, slider_state state) {

    // Default rendering for slider is just a stub for testing functionality before real rendering
    // has been implemented

    QPainter p(this);

    QColor bg(0, 0, 0);
    QColor c(180, 128, 128);

    switch(state) {
        case slider_state::normal:
            break;

        case slider_state::disabled:
            c = QColor(c.red(), c.green(), c.blue(), 64);
            bg = QColor(bg.red(), bg.green(), bg.blue(), 64);
            break;

        case slider_state::knob_hovered:
            c = c.lighter();
            bg = QColor(12, 12, 12);
            break;

        case slider_state::rect_hovered:
            bg = QColor(12, 12, 12);
            break;

        case slider_state::pressed:
            c = c.lighter().lighter();
            bg = QColor(12, 12, 12);
            break;
    }

    p.fillRect(rect(), bg);
    p.fillRect(knob, c);
}

inline void custom_slider::enterEvent([[maybe_unused]] QEnterEvent* event) {
    hovering = true;
    update();
}

inline void custom_slider::leaveEvent([[maybe_unused]] QEvent* event) {
    hovering = false;
    update();
}

inline void custom_slider::mouseMoveEvent(QMouseEvent* event) {
    last_move = event->pos();

    if(pressed) {
        const auto position = pixel_to_pos(last_move.x() - move_offset.x());
        set(position);
        emit changed();
    } else if(hovering) {
        update();
    }
}

inline void custom_slider::mousePressEvent(QMouseEvent* event) {
    pressed = true;
    const auto kr = knob_rect();

    if(kr.contains(event->pos())) {
        move_offset = event->pos() - kr.topLeft();
    } else {
        move_offset = kr.center() - kr.topLeft();
        custom_slider::mouseMoveEvent(event);
    }

    update();
}

inline void custom_slider::mouseReleaseEvent([[maybe_unused]] QMouseEvent* event) {
    pressed = false;
    update();
}

inline void custom_slider::focusOutEvent([[maybe_unused]] QFocusEvent* event) {
    if(pressed) {
        pressed = false;
        update();
    }
}



} // namespace cosel



#endif // cosel_qt_custom_slider_h
