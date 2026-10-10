// SPDX-License-Identifier: MIT
// Copyright (c) 2026 David Billen

// Implements Qt widget that displays/edits color types as text

#ifndef cosel_qt_info_widget_h
#define cosel_qt_info_widget_h

#include <vector>

#include <QColor>
#include <QFont>
#include <QGroupBox>
#include <QFontMetrics>
#include <QGridLayout>
#include <QLabel>
#include <QRegularExpressionValidator>
#include <QString>
#include <QWidget>

#include "cosel_qt_info.h"
#include "cosel_qt_slider_label.h"
#include "cosel_vec.h"

namespace cosel {

class info_widget : public QWidget {
    Q_OBJECT;
public:
    explicit info_widget(selected_color_t selection,
                         const info_options_t& options, QWidget* parent = nullptr) :
        QWidget(parent),
        selection_(std::move(selection)),
        data(options) {

        // Get an ID although currently we don't use it
        id = selection_->assign_id();

        // Create a grid layout and add the items to it
        auto* grid_layout = new QGridLayout();
        //grid_layout->setContentsMargins(0, 0, 0, 0);
        grid_layout->setSpacing(4);

        data.foreach_item([this, grid_layout](info_type_t& item) {
            add_row(grid_layout, item);
        });

        auto* box = new QGroupBox(this);
        box->setTitle(tr("Types"));
        box->setFlat(true);
        box->setLayout(grid_layout);
        auto* main_layout = new QVBoxLayout();
        main_layout->setContentsMargins(0, 0, 0, 0);
        main_layout->addWidget(box);
        setLayout(main_layout);

        setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

        // Handle the selected color signal
        connect(selection_.get(), &selected_color::changed, this, [this](unsigned) {
            // We do not ignore our own id because we are comprised of many values and updating
            // them in response to a change does not trigger another signal.
            handle_color_change();
        });

        // Set the selected color to start
        handle_color_change();
    }
private:
    using info_data_t = details::info_data;
    using info_type_t = details::info_type;

    selected_color_t selection_;
    unsigned id = 0;

    info_data_t data;

    struct row_t {
        triplet data;
        QLabel* label = nullptr;
        std::array<slider_label_edit *, 3> editbox = { nullptr, nullptr, nullptr };
    };

    std::vector<row_t> rows;
    QLineEdit* hex_edit = nullptr;

    void add_row(QGridLayout* grid_layout, info_type_t& item);
    void handle_color_change();

    void set_error(unsigned index, const xyz& color);
};

inline void info_widget::set_error(unsigned int index, const xyz& color) {
    auto& item = data.get(index);

    if(!item.in_gamut()) {
        // This item can specify a color out-of-gamut. Indicate an error if it is.
        const auto error = !rgb(color).in_gamut();
        auto* label = rows[index].label;

        if(item.error() != error) {
            item.set_error(error);

            if(error) {
                label->setText(QString(QStringLiteral("<span style='color: red;'>%1</span>")).arg(item.name()));
            } else {
                label->setText(item.name());
            }
        }
    }
}

inline void info_widget::add_row(QGridLayout* grid_layout, info_type_t& item) {
    const auto row_number = grid_layout->rowCount();

    row_t row;

    // Add the name of the item
    row.label = new QLabel();
    row.label->setMinimumWidth(konst::min_label_width());
    row.label->setMaximumWidth(konst::min_label_width());
    row.label->setText(item.name());
    row.label->setAlignment(Qt::AlignLeft);
    grid_layout->addWidget(row.label, row_number, 0, 1, 1);


    // Add the values
    const auto& lowrange = item.editrange_low();
    const auto& highrange = item.editrange_high();
    const auto index = item.index();

    for(int i = 0; i < 3; ++i) {
        // Create the edit widget and add to grid layout
        row.editbox[i] = new slider_label_edit(lowrange.comp[i], highrange.comp[i], item.precision(), this);
        grid_layout->addWidget(row.editbox[i], row_number, i + 1, 1, 1);

        // Connect the edit widget signal to handle changes
        connect(row.editbox[i], &slider_label_edit::edited, this, [this, index, i](double n) {
            const auto& item = data.get(index);
            // Set the individual color component that corresponds to this edit widget
            rows[index].data.comp[i] = n;
            // Calculate the resulting color and set it
            const auto color = item.to_xyz(rows[index].data);
            set_error(index, color);

            selection_->set_xyz(color, id);
        });
    }

    // Save the data
    assert(rows.size() == index);
    rows.emplace_back(row);
}

inline void info_widget::handle_color_change() {
    const auto xyz_c = selection_->xyz();

    data.foreach_item([this, xyz_c](info_type_t& item) {
        // Convert the XYZ color value to the triplet format that the item supports
        rows[item.index()].data = item.from_xyz(xyz_c);

        // Set or clear error indicator for out-of-gamut types
        set_error(item.index(), xyz_c);

        // Set each component
        for(int i = 0; i < 3; ++i) {
            rows[item.index()].editbox[i]->set(rows[item.index()].data.comp[i]);
        }

    });
}


} // namespace cosel


#endif // cosel_qt_info_widget_h
