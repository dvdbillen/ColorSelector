// SPDX-License-Identifier: MIT
// Copyright (c) 2026 David Billen

// Selected color - QObject for use with multiple selection/display components
//
// An instance of this class is required to define the current color for all of the cosel top level
// widgets, (as opposed to that color being defined in a top level dialog).


/// @file cosel_qt_selected_color.h
/// @brief Selected color class implementation

#ifndef cosel_qt_selected_color_h
#define cosel_qt_selected_color_h

#include <QObject>
#include <QColor>

#include "cosel_model.h"
#include "cosel_qt_app_defs.h"
#include "cosel_qt_tools.h"

namespace cosel {

/// @brief QObject based class for use with multiple color selection components.
///
/// @details An instance of this class is required to define the current color for all of the cosel
/// top level widgets.
///
/// Typically color selection dialogs or widgets contain the current color selection as a class
/// member. In cosel the current color selection is this discrete object. All of the classes that
/// set or respond to the current color selection are initialized with a shared_ptr to a single
/// instance of a selected_color.

class selected_color : public QObject {
    Q_OBJECT;

public:
    /// @brief Construct with an xyz initial color.
    ///
    /// @param init_color an xyz color value.
    explicit selected_color(const cosel::xyz& init_color) :
        color_(init_color),
        rgb_color_(cosel::rgb(init_color).clamped_gamut()) {}

    /// @brief Construct with an rgb initial color.
    ///
    /// @param init_color an rgb color value. It will be clamped into range.
    explicit selected_color(const cosel::rgb& init_color) :
        color_(cosel::xyz(init_color)),
        rgb_color_(init_color.clamped_gamut()) {}

    /// @brief Construct with a QColor initial color (including alpha).
    ///
    /// @param init_color a QColor color value.
    explicit selected_color(const QColor& init_color) :
        selected_color(qt_to_rgb(init_color)) {
        alpha_ = init_color.alphaF();
    }

    /// @brief Obtain a unique ID to use when handling color changes.
    ///
    /// @details This helps classes to avoid comparing floating points values in order to ignore
    /// changes made by themselves. It is specified when setting a color value and is contained in
    /// the changed signal.
    ///
    /// @return An unsigned integer that is unique to the application.
    unsigned assign_id() {
        return ++id_odo;
    }

    /// @brief Get the current color in xyz.
    ///
    /// @return An xyz color that may be out of gamut in the color space.
    cosel::xyz xyz() const {
        return color_;
    }

    /// @brief Get the current color in rgb.
    ///
    /// @return An rgb color that is in gamut in the color space.
    cosel::rgb rgb() const {
        return rgb_color_;
    }

    /// @brief Get the current color as a Qt QColor (including alpha)
    ///
    /// @return A valid QColor.
    QColor qcolor() const {
        auto c = qt_from_rgb(rgb_color_);
        c.setAlphaF(alpha_);
        return c;
    }

    /// @brief Get the current alpha.
    ///
    /// @return Last alpha value set in range 0..1.
    double alpha() const {
        return alpha_;
    }

    /// @brief Set the current color specified as xyz.
    ///
    /// @param color An xyz color that is not required to be in gamut.
    /// @param id A unique id that was obtained using the @ref assign_id static() method.
    void set_xyz(const cosel::xyz& color, unsigned id) {
        color_ = color;
        rgb_color_ = cosel::rgb(color_).clamped_gamut();

        emit changed(id);
    }

    /// @brief Set the current color specified as rgb.
    ///
    /// @param color An rgb color. It is forced valid if it isn't already.
    /// @param id A unique id that was obtained using the @ref assign_id static() method.
    void set_rgb(const cosel::rgb& color, unsigned id) {
        color_ = cosel::xyz(color);
        rgb_color_ = color.clamped_gamut();

        emit changed(id);
    }

    /// @brief Set the current color specified a Qt QColor (including alpha).
    ///
    /// @param color A QColor.
    /// @param id A unique id that was obtained using the @ref assign_id static() method.
    void set_qcolor(const QColor& color, unsigned id) {
        set_rgb(qt_to_rgb(color), id);
        set_alpha(color.alphaF(), id);
    }

    /// @brief Set the current alpha.
    ///
    /// @param n A normal alpha value in range 0..1.
    /// @param id A unique id that was obtained using the @ref assign_id static() method.
    void set_alpha(double n, unsigned id) {
        const auto a = clamp(n);

        if(a != alpha_) {
            alpha_ = a;
            emit alpha_changed(id);
        }
    }

signals:
    /// @brief Emitted when a color has been set.
    ///
    /// @param id The ID that was passed to the set function.
    void changed(unsigned id);

    /// @brief Emitted when an alpha value has been set.
    ///
    /// @param id The ID that was passed to the set function.
    ///
    /// @details Most things don't reflect alpha so it uses a discrete signal.
    void alpha_changed(unsigned id);

private:
    cosel::xyz color_;
    cosel::rgb rgb_color_;
    double alpha_ = 1.0;
    static unsigned id_odo;
};

inline unsigned selected_color::id_odo = 0;
/// A shared_ptr to an instance of a selected_color
using selected_color_t = std::shared_ptr<selected_color>;


} // namespace cosel


#endif // cosel_qt_selected_color_h
