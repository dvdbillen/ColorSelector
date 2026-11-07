// SPDX-License-Identifier: MIT
// Copyright (c) 2026 David Billen

// Palette widget


/// @file cosel_qt_palette_h
/// @brief Palette and palette widget implementation

#ifndef cosel_qt_palette_h
#define cosel_qt_palette_h

#include <array>
#include <memory>
#include <string>

#include <QBoxLayout>
#include <QDebug>
#include <QGridLayout>
#include <QGroupBox>
#include <QString>
#include <QWidget>

#include "cosel_space.h"
#include "cosel_gradient.h"
#include "cosel_model.h"
#include "cosel_model_hsx.h"
#include "cosel_qt_app_defs.h"
#include "cosel_qt_selected_color.h"
#include "cosel_qt_swatch.h"
#include "cosel_qt_tools.h"
#include "cosel_space.h"

namespace cosel {

constexpr size_t palette_cols = 6;
constexpr size_t palette_standard_rows = 4;
constexpr size_t palette_custom_rows = 4;
/// @ingroup misc
/// @brief Constant specifying the number of standard colors
constexpr size_t palette_standard_size = palette_standard_rows*palette_cols;
/// @ingroup misc
/// @brief Constant specifying the number of custom colors
constexpr size_t palette_custom_size = palette_custom_rows*palette_cols;
constexpr size_t palette_rows = palette_standard_rows + palette_custom_rows;
/// @ingroup misc
/// @brief Constant specifying the total number of colors in a palette
constexpr size_t palette_size = palette_standard_size + palette_custom_size;


// Palette data representation. The widget is defined below it.

/// @brief Palette class
///
/// @details The color_palette class is not a singleton, but is not a general purpose class. It is
/// designed for use by the palette_widget class and the cosel dialog with a hard coded number of
/// custom and standard color.

class color_palette {
public:

    /// @brief Palette color class
    /// @details For color space support the colors are stored as both xyz and rgb values.
    struct color_t {
        /// Construct an empty (unused) color.
        color_t() = default;
        /// Construct with xyz
        /// @param color_ an XYZ color.
        explicit color_t(const xyz& color_) :
            color_(color_),
            display_color_(rgb(color_).clamped_gamut()),
            used_(true) {}
        /// Construct with rgb
        /// @param color_ an RGB color.
        explicit color_t(const rgb& color_) :
            color_(xyz(color_)),
            display_color_(color_),
            used_(true) {}

        /// Return color
        /// @return An XYZ color.
        const xyz& color() const {
            return color_;
        }

        /// Return RGB color
        /// @return An RGB color that is clamped to gamut even if the class was constructed with
        /// an out-of-gamut XYZ color.
        const rgb& display_color() const {
            return display_color_;
        }

        /// Indicates whether or not the color is used.
        /// @return true if the color was initialized with an empty constructor or the clear()
        /// function was used.
        const bool& used() const {
            return used_;
        }

        /// Set the color to unused.
        void clear() {
            used_ = false;
        }
    private:
        xyz color_;
        rgb display_color_;
        bool used_ = false;
    };

    /// Consstruct a palette
    ///
    /// @details The palette is always large enough to contain all of the standard and custom
    /// colors of sizes palette_standard_size and palette_custom_size. The custom colors are at
    /// offset palette_standard_size. On construction, the standard colors are initialized with
    /// hard-coded values, and the custom colors are empty.
    color_palette();

    /// Return a definition of the custom colors as a string.
    /// @return A QString that represents a definition of all the custom colors. It can be saved
    /// and the custom colors can be restored with the set_custom_color_string() member.
    [[nodiscard]] QString custom_color_string() const;


    /// Set the custom colors from a string.
    /// @param palstr A QString obtained using the custom_color_string() member.
    /// @return false if there is an error parsing the string.
    bool set_custom_color_string(const QString& palstr);

    /// Return a color_t by index.
    /// @param index in the range 0 to palette_size - 1.
    /// @return A color_t.
    const color_t& get(size_t index) const {
        return colors[std::min(index, colors.size() - 1)];
    }
    /// Set a palette color to an xyz value.
    /// @param index in the range 0 to palette_size - 1.
    /// @param color an XYZ color.
    void set(size_t index, const xyz& color) {
        colors[std::min(index, colors.size() - 1)] = color_t(color);
    }
    /// Set a palette color to an rgb value.
    /// @param index in the range 0 to palette_size - 1.
    /// @param color an RGB color.
    void set(size_t index, const rgb& color) {
        colors[std::min(index, colors.size() - 1)] = color_t(color);
    }


private:
    std::vector<color_t> colors;
};

inline color_palette::color_palette() :
    colors(palette_size, color_t()) {

    const std::array<lab, palette_standard_size> macbeth_colorchecker_lab_d50
    = {{
            // Row 1 (natural colors)
            lab{37.99,  13.56,  14.06},  // 1  dark skin
            lab{65.71,  18.13,  17.81},  // 2  light skin
            lab{49.93,  -4.88, -21.93},  // 3  blue sky
            lab{43.14, -13.10,  21.91},  // 4  foliage
            lab{55.11,   8.84, -25.40},  // 5  blue flower
            lab{70.72, -33.40,  -0.20},  // 6  bluish green

            // Row 2 (additive/subtractive + skin-ish)
            lab{62.66,  36.07,  57.10},  // 7  orange
            lab{40.02,  10.41, -45.96},  // 8  purplish blue
            lab{51.12,  48.24,  16.25},  // 9  moderate red
            lab{30.33,  22.98, -21.59},  // 10 purple
            lab{72.53, -23.71,  57.26},  // 11 yellow green
            lab{71.94,  19.36,  67.86},  // 12 orange yellow

            // Row 3 (primaries + secondaries)
            lab{28.78,  14.18, -50.30},  // 13 blue
            lab{55.26, -38.34,  31.37},  // 14 green
            lab{42.10,  53.38,  28.19},  // 15 red
            lab{81.73,   4.04,  79.82},  // 16 yellow
            lab{51.94,  49.99, -14.57},  // 17 magenta
            lab{51.04, -28.63, -28.64},  // 18 cyan

            // Row 4 (grays)
            lab{96.54,  -0.43,   1.19},  // 19 white 9.5 (.05 D)
            lab{81.26,  -0.64,  -0.34},  // 20 neutral 8 (.23 D)
            lab{66.77,  -0.73,  -0.50},  // 21 neutral 6.5 (.44 D)
            lab{50.87,  -0.15,  -0.27},  // 22 neutral 5 (.70 D)
            lab{35.66,  -0.42,  -1.23},  // 23 neutral 3.5 (1.05 D)
            lab{20.46,  -0.08,  -0.97}   // 24 black 2 (1.5 D)
        }
    };

    constexpr triplet d50{0.96422, 1.00000, 0.82521};

    for(unsigned i = 0; i < palette_standard_size; ++i) {
        const auto& lab_c = macbeth_colorchecker_lab_d50[i];
        const auto xyz_c_d50 = xyz(lab_c, d50);
        const auto xyz_c = xyz_c_d50.chromatic_adapt(d50, color_space().gamut().xyz_white());

        colors[i] = color_t(xyz_c);
    }
}

inline QString color_palette::custom_color_string() const {
    const auto comma = QStringLiteral(",");
    const auto newline = QStringLiteral("\n");
    const auto empty = QStringLiteral("empty\n");

    QString palstr;

    for(size_t i = palette_standard_size; i < palette_size; ++i) {
        if(colors[i].used()) {
            const auto& c = colors[i].color();
            QString x = QString(QStringLiteral("%1")).arg(c.X(), 0, 'f', 6);
            QString y = QString(QStringLiteral("%1")).arg(c.Y(), 0, 'f', 6);
            QString z = QString(QStringLiteral("%1")).arg(c.Z(), 0, 'f', 6);

            palstr += x + comma + y + comma + z + newline;
        } else {
            palstr += empty;
        }
    }

    return palstr;
}

inline bool color_palette::set_custom_color_string(const QString& palstr) {
    assert(colors.size() == palette_size);

    const auto comma = QStringLiteral(",");
    const auto newline = QStringLiteral("\n");
    const auto empty = QStringLiteral("empty");

    QStringList lines = palstr.split(newline);

    size_t count = palette_standard_size;
    bool ok = false;

    for(const QString& line : std::as_const(lines)) {
        if(line == empty) {
            colors[count] = color_t();
        } else {

            QStringList components = line.split(QStringLiteral(","));

            if(components.size() != 3) {
                return false;
            }

            triplet data;

            for(size_t i = 0; i < 3; ++i) {
                data.comp[i] = components[i].toDouble(&ok);

                if(!ok) {
                    return false;
                }
            }

            colors[count] = color_t(xyz(data));
        }

        if(++count == palette_size) {
            break;
        }
    }

    return true;
}


////////////////////////////////////////////////////////////////////////////////////////////////////

/// @brief Palette widget
///
/// @details A palette_widget is designed for use by the the cosel dialog with a hard coded number
/// of custom and standard color.

class palette_widget : public QWidget {
    Q_OBJECT;
public:
    /// Construct a palette_widget specifying an existing palette.
    /// @param palette A shared_ptr to a color_palette.
    /// @param selection_ A shared_ptr to a selected_color.
    /// @param corner_rounding_ A corner rounding value that works as defined for the
    /// gradient_slider::style_options_t class.
    explicit palette_widget(const std::shared_ptr<color_palette>& palette,
                            selected_color_t selection_, double corner_rounding_);

    /// Construct a palette_widget that creates its own palette.
    /// @param selection_ A shared_ptr to a selected_color.
    /// @param corner_rounding_ A corner rounding value that works as defined for the
    /// gradient_slider::style_options_t class.
    explicit palette_widget(selected_color_t selection_, double corner_rounding_) :
        palette_widget(std::make_shared<color_palette>(), std::move(selection_), corner_rounding_) {}

    /// Return the associated palette.
    /// @return A constant reference to the color_palette.
    const color_palette& palette() const {
        return *palette_.get();
    }

    /// Return the palette size.
    /// @return The palette size. This value is defined in a constant but this function exists
    /// anyway to remain flexible.
    size_t size() const {
        return palette_size;
    }

    /// Return a string representation of the custom colors.
    /// @return A QString that represents a definition of all the custom colors. See @ref
    /// color_palette::custom_color_string().
    [[nodiscard]] QString custom_color_string() const {
        return palette_->custom_color_string();
    }

    /// Set the custom colors from a string.
    /// @param palstr A QString obtained using the custom_color_string() member. See @ref
    /// color_palette::set_custom_color_string().
    /// @return false if there is an error parsing the string.
    bool set_custom_color_string(const QString& palstr);

signals:

    /// @public
    /// @brief Signal emitted when the custom palette changes, typically as the result of a
    /// drag-and-drop.
    void custom_palette_changed();

private:
    std::shared_ptr<color_palette> palette_;
    xyz empty_color;
    selected_color_t selection;
    unsigned selection_id = 0;
    double corner_rounding = 0;

    std::vector<color_swatch*> swatches;

    void init();
};


inline palette_widget::palette_widget(const std::shared_ptr<color_palette>& palette,
                                      selected_color_t selection_, double corner_rounding_) :
    palette_(palette),
    selection(std::move(selection_)),
    corner_rounding(corner_rounding_) {
    init();
}

inline bool palette_widget::set_custom_color_string(const QString& palstr) {
    if(!palette_->set_custom_color_string(palstr)) {
        return false;
    }

    for(unsigned i = palette_standard_size; i < palette_size; ++i) {
        const auto c = palette_->get(i);

        if(c.used()) {
            swatches[i]->set(c.color());
        } else {
            swatches[i]->clear();
        }
    }

    return true;
}

inline void palette_widget::init() {
    auto bgfg = get_bg_fg_colors(this);

    const auto empty_rgb = qt_to_rgb(bgfg.first);
    empty_color = xyz(empty_rgb);

    selection_id = selection->assign_id();

    auto* standard_layout = new QGridLayout();
    auto* custom_layout = new QGridLayout();

    for(unsigned i = 0; i < palette_standard_size; ++i) {
        const auto& c = palette_->get(i);

        if(c.used()) {
            swatches.emplace_back(new color_swatch(c.display_color(), corner_rounding, false));
        } else {
            swatches.emplace_back(new color_swatch(corner_rounding, false));
        }

        swatches.back()->setFixedSize(konst::palette_swatch_size(), konst::palette_swatch_size());

        connect(swatches[i], &color_swatch::clicked, this, [this, i]() {
            selection->set_rgb(palette_->get(size_t(i)).display_color(), selection_id);
        });

        standard_layout->addWidget(swatches.back(),
                                   i/palette_cols, i%palette_cols, 1, 1);
    }

    for(unsigned i = palette_standard_size; i < palette_size; ++i) {
        const auto& c = palette_->get(i);

        if(c.used()) {
            swatches.emplace_back(new color_swatch(c.display_color(),
                                                   corner_rounding, true));
        } else {
            swatches.emplace_back(new color_swatch(corner_rounding, true));
        }

        swatches.back()->setFixedSize(konst::palette_swatch_size(), konst::palette_swatch_size());

        connect(swatches.back(), &color_swatch::clicked, this, [this, i]() {
            selection->set_rgb(palette_->get(size_t(i)).display_color(), selection_id);
        });

        connect(swatches.back(), &color_swatch::dropped, this, [this, i]() {
            palette_->set(i, xyz(swatches[i]->get()));
            emit custom_palette_changed();
        });

        custom_layout->addWidget(swatches.back(),
                                 i/palette_cols - palette_standard_rows, i%palette_cols, 1, 1);
    }

    auto* standard_groupbox = new QGroupBox();
    standard_groupbox->setTitle(tr("Standard Colors"));
    standard_groupbox->setFlat(true);
    standard_groupbox->setLayout(standard_layout);

    auto* custom_groupbox = new QGroupBox();
    custom_groupbox->setTitle(tr("Custom Colors"));
    custom_groupbox->setFlat(true);
    custom_groupbox->setLayout(custom_layout);

    auto* v_layout = new QVBoxLayout();
    v_layout->setContentsMargins(0, 0, 0, 0);
    v_layout->addWidget(standard_groupbox, 0, Qt::AlignTop);

    auto* layout = new QHBoxLayout();
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addLayout(v_layout);
    layout->addWidget(custom_groupbox, 0, Qt::AlignTop);

    setLayout(layout);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
}


} // namespace cosel


#endif // cosel_qt_palette_h
