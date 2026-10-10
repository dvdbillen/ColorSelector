// SPDX-License-Identifier: MIT
// Copyright (c) 2026 David Billen

// Implements Qt widget that displays/edits sRGB HTML

#ifndef cosel_qt_html_widget_h
#define cosel_qt_html_widget_h

#include <vector>

#include <QColor>
#include <QFont>
#include <QGroupBox>
#include <QFontMetrics>
#include <QLabel>
#include <QString>
#include <QWidget>

#include "cosel_qt_info.h"
#include "cosel_qt_slider_label.h"
#include "cosel_vec.h"

namespace cosel {

class html_widget : public QWidget {
    Q_OBJECT;
public:
    explicit html_widget(selected_color_t selection, QWidget* parent = nullptr) :
        QWidget(parent),
        id(selection->assign_id()),
        selection_(std::move(selection)) {

        auto* layout = new QHBoxLayout();
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(0);

        hex_edit = make_widget();
        auto* label = new QLabel(tr("HTML #"));
        layout->addWidget(label, 0, Qt::AlignRight);
        layout->addWidget(hex_edit, 0, Qt::AlignLeft);

        setLayout(layout);
        setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

        // Handle the selected color signal
        connect(selection_.get(), &selected_color::changed, this, [this](unsigned id_) {
            if(id != id_) {
                handle_color_change();
            }
        });

        // Set the selected color to start
        defer(0, [this]() {
            handle_color_change();
        });
    }
private:
    unsigned id = 0;
    selected_color_t selection_;
    QLineEdit* hex_edit = nullptr;
    QLineEdit* make_widget();
    void handle_color_change();
};

inline QLineEdit* html_widget::make_widget() {

    auto* edit = new QLineEdit(this);
    edit->setFrame(false);
    edit->setMaximumWidth(konst::max_edit_width());
    edit->setAlignment(Qt::AlignCenter);

    auto* validator = new QRegularExpressionValidator(this);
    QRegularExpression rx{QStringLiteral("^([0-9A-Fa-f]{3}){1,2}$")};
    validator->setRegularExpression(rx);
    edit->setValidator(validator);

    connect(edit, &QLineEdit::editingFinished, edit, [edit, this]() {
        auto value = edit->text();

        // Convert to xyz always using sRGB color space
        const auto c = QColor(QStringLiteral("#") + value);
        const auto html_d = triplet(
                                srgb_colorspace().to_linear(c.redF()),
                                srgb_colorspace().to_linear(c.greenF()),
                                srgb_colorspace().to_linear(c.blueF())
                            );
        const auto xyz_d = srgb_colorspace().gamut().rgb2xyz(html_d);
        selection_->set_xyz(xyz(xyz_d), id);
    });

    return edit;
}

inline void html_widget::handle_color_change() {
    if(hex_edit == nullptr) {
        return;
    }

    const auto xyz_c = selection_->xyz();

    // Convert to sRGB color space

    // TODO: If the color is out-of-gamut this currently results in clamped components. Consider
    // making the rgb class function that clamps to gamut accessible with a specified color space
    // so that this color can clamp to the sRGB gamut more gracefully.

    const auto rgb_d = srgb_colorspace().gamut().xyz2rgb(xyz_c.data());
    const auto r = clamp(srgb_colorspace().to_gamma(rgb_d.comp[0]));
    const auto g = clamp(srgb_colorspace().to_gamma(rgb_d.comp[1]));
    const auto b = clamp(srgb_colorspace().to_gamma(rgb_d.comp[2]));
    QColor c;
    c.setRgbF(r, g, b);
    auto name = c.name(QColor::HexRgb);

    if(name.startsWith(QStringLiteral("#"))) {
        name.remove(0, 1);
    }

    hex_edit->setText(name.toUpper());
}


} // namespace cosel


#endif // cosel_qt_html_widget_h

