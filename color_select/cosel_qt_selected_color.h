// SPDX-License-Identifier: MIT
// Copyright (c) 2026 David Billen

// Selected color

// Color for selectors is a QObject for use with multiple selection/display components


#ifndef cosel_qt_selected_color_h
#define cosel_qt_selected_color_h

#include <QObject>
#include <QColor>

#include "cosel_model.h"
#include "cosel_qt_app_defs.h"
#include "cosel_qt_tools.h"

namespace cosel {

class selected_color : public QObject {
    Q_OBJECT;

public:
    explicit selected_color(const cosel::xyz& init_color) :
        color_(init_color),
        rgb_color_(cosel::rgb(init_color).clamped_gamut()) {}

    explicit selected_color(const cosel::rgb& init_color) :
        color_(cosel::xyz(init_color)),
        rgb_color_(init_color.clamped_gamut()) {}

    explicit selected_color(const QColor& init_color) :
        selected_color(qt_to_rgb(init_color)) {}

    unsigned assign_id() {
        return ++id_odo;
    }

    // This could be out of gamut.
    cosel::xyz xyz() const {
        return color_;
    }

    // This is clamped to gamut.
    cosel::rgb rgb() const {
        return rgb_color_;
    }

    QColor qcolor() const {
        return qt_from_rgb(rgb_color_);
    }

    double alpha() const {
        return alpha_;
    }

    void set_xyz(const cosel::xyz& color, unsigned id) {
        color_ = color;
        rgb_color_ = cosel::rgb(color_).clamped_gamut();

        emit changed(id);
    }

    void set_rgb(const cosel::rgb& color, unsigned id) {
        color_ = cosel::xyz(color);
        rgb_color_ = color.clamped_gamut();

        emit changed(id);
    }

    void set_qcolor(const QColor& color, unsigned id) {
        set_rgb(qt_to_rgb(color), id);
    }

    void set_alpha(double n, unsigned id) {
        alpha_ = clamp(n);

        emit alpha_changed(id);
    }

signals:
    void changed(unsigned id);
    // Most things don't reflect alpha so it's a discrete signal
    void alpha_changed(unsigned id);

private:
    cosel::xyz color_;
    cosel::rgb rgb_color_;
    double alpha_ = 1.0;
    static unsigned id_odo;
};

inline unsigned selected_color::id_odo = 0;
using selected_color_t = std::shared_ptr<selected_color>;


} // namespace cosel


#endif // cosel_qt_selected_color_h
