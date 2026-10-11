#include "cosel_demo_widget.h"

#include <memory>
#include <vector>

#include <QColorDialog>

#include "color_select/cosel_qt_dialog.h"
#include "unit_test.h"

constexpr bool use_unit_test = false;

////////////////////////////////////////////////////////////////////////////////////////////////////
// Execute the standard QColorDialog for comparison

void cosel_demo_widget::handle_qtdialog() {
    const auto current = cosel::qt_from_rgb(the_color->rgb());
    QColorDialog dialog(current, this);

    dialog.setOption(QColorDialog::ShowAlphaChannel, true);
    dialog.setOption(QColorDialog::DontUseNativeDialog, false);

    if(dialog.exec() == QDialog::Accepted) {
        the_color->set_qcolor(dialog.selectedColor(), -1);
    }
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// Execute our mock dialog

void cosel_demo_widget::handle_mock_dialog() {
    const auto current = cosel::qt_from_rgb(the_color->rgb());
    cosel::color_dialog dialog(current, this);

    // We could also set the extended options as we do below in handle_coseldialog(), but here we
    // are demonstrating plug-in replacement for QColorDialog.

    dialog.setOption(QColorDialog::ShowAlphaChannel, true);
    dialog.setOption(QColorDialog::DontUseNativeDialog, false);

    if(dialog.exec() == QDialog::Accepted) {
        the_color->set_qcolor(dialog.selectedColor(), -1);
    }
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// Use the widget directly

void cosel_demo_widget::handle_coseldialog() {

    // Set every single option to a value for demonstration (and to make it easy to experiment).
    // They all have defaults so you don't actually have to set any of them. These options can also
    // be used with the cosel::color_dialog class by invoking its set_cosel_options function or
    // using the version of its constructor that accepts them.


    // Slider options - these apply to all color selector sliders. May also be used for color
    // swatches where appropriate (e.g. rounding).
    cosel::gradient_slider::style_options_t options;

    // Set the height of the sliders, (width is sizeable at runtime)
    options.height = cosel::konst::default_slider_height();

    // If true the slider pointers are on top. Default is on bottom
    options.pointer_top = false;

    // Set corner rounding. This varies from 0 (no rounding) to 1 (fully circular).
    options.rounding = cosel::konst::default_corner_rounding();

    // Present sliders with labels. If set to false then neither a descriptive label or an editable
    // value label is used. This option is intended for use of the slider classes directly and is
    // not useful in our widget.
    options.info = true;


    // Widget options

    cosel::select_widget::options_t select_options;

    // The slider options are a widget option. Use the ones we set above.
    select_options.slider_style = options;

    // Each type of color model selector slider group can be enabled using
    // cosel::select_widget::options_t.slider_enables. However, that involves indexing by casting
    // an enum. There is a helper function corresponding to each type to make it easier.
    //
    // Each enabled set of sliders adds a tab. If only one type is enabled then there are no tabs.
    // It is supported to have none of the sliders enabled.

    select_options.enable_rgb(true);
    select_options.enable_hsv(true);
    select_options.enable_hsl(true);
    select_options.enable_hsv2(true);
    select_options.enable_hsl2(true);

    // Enable the alpha channel slider.
    select_options.use_alpha = false;

    // Enable the screen picker.
    select_options.use_screen_picker = true;

    // Enable the 2D color selectors. This is the HSV ring selector and the color grid selector
    // inside a tab control.
    select_options.two_d_widget = true;

    // Set the resolution as a pixel value that affects width and height of each color. If you do
    // not want the color selector to be a grid and instead want a conventional detailed selector
    // then set this to 1.
    select_options.two_d_resolution = cosel::konst::default_2d_resolution();

    // When using a grid of size >= 4 this flag indicates each segment should be outlined. This
    // is the default but may cause artifacts on some platforms depending on the resolution.
    select_options.two_d_bordered = cosel::konst::default_2d_bordered();


    // Make a dialog and use our widget as the contents.

    // Note that construction specifies the initial selector types and twod type. If the specified
    // cosel::select_widget::state_t is initialized then it will override the constructor options.
    const auto initial_slider_type = cosel::selector_type_t::rgb;
    const auto initial_twod_type = cosel::twod_type_t::ring;

    auto* selector = new cosel::select_widget(select_options,
            initial_slider_type, initial_twod_type,
            the_color, cosel_state, this);


    // Connect the state change signal. This is a way to track state changes such as which tab is
    // selected as well as changes to custom colors. It is not required to use this signal. If the
    // widget is still accessible after the selection is complete then you can read the state from
    // the class instance. In this demo code however, the widget is going to be parented by the
    // dialog and deleted after exec(). We won't have that oppotunity.
    connect(selector, &cosel::select_widget::state_changed,
    this, [this](const cosel::select_widget::state_t& state) {
        cosel_state = state;
    });


    auto* main_layout = new QVBoxLayout();
    main_layout->addWidget(selector);

    // Exec the dialog

    QDialog dialog;
    dialog.setLayout(main_layout);
    dialog.setMinimumWidth(640);

    dialog.exec();
}

void cosel_demo_widget::paintEvent([[maybe_unused]] QPaintEvent* event) {
}

cosel_demo_widget::cosel_demo_widget(QWidget* parent)
    : QWidget{parent} {

    the_color = std::make_shared<cosel::selected_color>(
                    cosel::xyz(cosel::rgb(1, 0, 0)));

    auto* toolbar = new QToolBar();
    toolbar->setFixedHeight(28);

    qtdialog = new QAction(tr("Qt Dialog"), this);
    qtdialog->setToolTip(tr("Show QColorDialog for comparison"));
    connect(qtdialog, &QAction::triggered, this, [this]() {
        handle_qtdialog();
    });

    toolbar->addAction(qtdialog);

    mockdialog = new QAction(tr("Mock Dialog"), this);
    mockdialog->setToolTip(tr("Show Cosel QColorDialog replacement"));
    connect(mockdialog, &QAction::triggered, this, [this]() {
        handle_mock_dialog();
    });

    toolbar->addAction(mockdialog);

    coseldialog = new QAction(tr("Cosel Dialog"), this);
    coseldialog->setToolTip(tr("Show Cosel dialog with different options"));
    connect(coseldialog, &QAction::triggered, this, [this]() {
        handle_coseldialog();
    });

    toolbar->addAction(coseldialog);

    if constexpr(use_unit_test) {
        unittest = new QAction(tr("Unit Test"), this);
        connect(unittest, &QAction::triggered, this, [this]() {
            setEnabled(false);
            unit_test();
            setEnabled(true);
        });

        toolbar->addAction(unittest);
    }

    auto* main_layout = new QVBoxLayout();
    main_layout->addWidget(toolbar);
    main_layout->addStretch();

    setLayout(main_layout);

}
