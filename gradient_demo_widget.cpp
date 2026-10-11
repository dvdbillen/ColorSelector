#include "gradient_demo_widget.h"

#include <memory>
#include <vector>

#include <QColorDialog>

#include "outline_tester.h"

#include "color_select/cosel_qt_dialog.h"

#include "unit_test.h"

void gradient_demo_widget::handle_qtdialog() {
    const auto current = cosel::qt_from_rgb(the_color->rgb());
    QColorDialog dialog(current, this);
    dialog.setOption(QColorDialog::ShowAlphaChannel, true);
    dialog.setOption(QColorDialog::DontUseNativeDialog, false);

    if(dialog.exec() == QDialog::Accepted) {
        the_color->set_qcolor(dialog.selectedColor(), -1);
    }
}

void gradient_demo_widget::handle_mock_dialog() {
    const auto current = cosel::qt_from_rgb(the_color->rgb());
    cosel::color_dialog dialog(current, this);

    dialog.setOption(QColorDialog::ShowAlphaChannel, false);
    dialog.setOption(QColorDialog::DontUseNativeDialog, false);
    auto opts = dialog.cosel_options();
    opts.two_d_widget = true;
    opts.use_screen_picker = true;
    opts.enable_hsv2(true);
    opts.enable_hsl2(true);
    dialog.set_cosel_options(opts);

    if(dialog.exec() == QDialog::Accepted) {
        the_color->set_qcolor(dialog.selectedColor(), -1);
    }
}

void gradient_demo_widget::handle_coseldialog() {

    cosel::gradient_slider::style_options_t options;

    cosel::select_widget::options_t select_options;
    select_options.slider_style = options;
    select_options.slider_enables[size_t(cosel::selector_type_t::rgb)] = true;
    select_options.slider_enables[size_t(cosel::selector_type_t::hsl)] = true;
    select_options.slider_enables[size_t(cosel::selector_type_t::hsv)] = true;
    select_options.slider_enables[size_t(cosel::selector_type_t::hsl2)] = true;
    select_options.slider_enables[size_t(cosel::selector_type_t::hsv2)] = true;

    select_options.use_alpha = false;
    select_options.use_screen_picker = true;
    select_options.two_d_widget = true;
    select_options.two_d_resolution = cosel::konst::default_2d_resolution();
    select_options.two_d_bordered = cosel::konst::default_2d_bordered();

    QDialog dialog;
    auto* main_layout = new QVBoxLayout();
    auto* selector = new cosel::select_widget(select_options,
            cosel::selector_type_t::rgb, cosel::twod_type_t::ring,
            the_color, cosel_state, this);

    connect(selector, &cosel::select_widget::state_changed,
    this, [this](const cosel::select_widget::state_t& state) {
        cosel_state = state;
    });

    main_layout->addWidget(selector);

    dialog.setLayout(main_layout);
    dialog.setMinimumWidth(640);
    dialog.exec();
}

void gradient_demo_widget::paintEvent([[maybe_unused]] QPaintEvent* event) {
}

gradient_demo_widget::gradient_demo_widget(QWidget* parent)
    : QWidget{parent} {

    the_color = std::make_shared<cosel::selected_color>(
                    cosel::xyz(cosel::rgb(1, 0, 0)));

    auto* toolbar = new QToolBar();
    toolbar->setFixedHeight(28);

    qtdialog = new QAction(tr("Qt Dialog"), this);
    connect(qtdialog, &QAction::triggered, this, [this]() {
        handle_qtdialog();
    });

    toolbar->addAction(qtdialog);

    mockdialog = new QAction(tr("Mock Dialog"), this);
    connect(mockdialog, &QAction::triggered, this, [this]() {
        handle_mock_dialog();
    });

    toolbar->addAction(mockdialog);

    coseldialog = new QAction(tr("Cosel Dialog"), this);
    connect(coseldialog, &QAction::triggered, this, [this]() {
        handle_coseldialog();
    });

    toolbar->addAction(coseldialog);

    unittest = new QAction(tr("Unit Test"), this);
    connect(unittest, &QAction::triggered, this, [this]() {
        setEnabled(false);
        unit_test();
        setEnabled(true);
    });

    toolbar->addAction(unittest);

    auto* main_layout = new QVBoxLayout();
    main_layout->addWidget(toolbar);
    main_layout->addStretch();

    setLayout(main_layout);

}
