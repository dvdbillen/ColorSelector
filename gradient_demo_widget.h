#ifndef GRADIENT_DEMO_WIDGET_H
#define GRADIENT_DEMO_WIDGET_H

#include <array>
#include <vector>

#include <QAction>
#include <QColor>
#include <QToolBar>
#include <QWidget>

#include "color_select/cosel_model.h"
#include "color_select/cosel_qt_select_widget.h"


class gradient_demo_widget : public QWidget {
    Q_OBJECT

    constexpr static size_t gradient_count  = 4;
    constexpr static int gradient_height = 48;
    constexpr static int gradient_spacing = 2;
    constexpr static std::array<int, gradient_count> gradient_rez = { 50, 1, 50, 1 };
    constexpr static std::array<bool, gradient_count> gradient_rgb = { true, true, false, false };
    constexpr static std::array<const char*, gradient_count> gradient_tag = { "Linear", "", "Uniform", "" };

    QToolBar* toolbar = nullptr;
    QAction* qtdialog = nullptr;
    QAction* mockdialog = nullptr;
    QAction* coseldialog = nullptr;
    QAction* unittest = nullptr;
    cosel::selected_color_t the_color;
    cosel::select_widget::state_t cosel_state;

    void handle_qtdialog();
    void handle_mock_dialog();
    void handle_coseldialog();
protected:
    void paintEvent([[maybe_unused]] QPaintEvent* event) override;
public:

    explicit gradient_demo_widget(QWidget* parent = nullptr);


signals:

};

#endif // GRADIENT_DEMO_WIDGET_H
