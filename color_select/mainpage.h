/**

@mainpage Cosel Qt color selector

@section intro_sec Introduction

This Doxygen documentation is to help Qt developers use the widget classes that are available for
an application independent of the QColorDialog compatible @ref cosel::color_dialog.

All other classes are documented with source code comments for developers who need to understand
the code.

@section concepts Concepts

<strong>The color_dialog and the select_widget</strong>

The cosel::color_dialog allows color selection with the same interface as QColorDialog. Internally,
the cosel::color_dialog simply creates a cosel::select_widget to implement the controls. You can
create a cosel::select_widget without using the cosel::color_dialog in order to handle your own
dialog or for use outside of a dialog.

Inside the cosel::select_widget are a number of other widgets. Many of these can also be used
independently and the purpose of this Doxygen documentation is to define the interfaces. Most of
them simply need to be constructed and placed in a layout. See @ref toplevel


<strong>The Selected Color</strong>

A QColorDialog contains a class member that holds the color being selected. It provides access to
that color as well as signals to indicate when it changes. Cosel appears to work identically when
accessed through its QColorDialog mock interface, but internally it works differently.

The cosel cosel::color_dialog and cosel::select_widget do not define the current color selection.
Instead, the current color is contained in a discrete class @ref cosel::selected_color and accessed
by all widgets that participate in the color selection using a shared_ptr to a single instance.

@note All of the top-level widgets that can update or respond to changes require a shared_ptr to a
selected_color object to be passed to their constructor.


<strong>State Data</strong>

It is a nice feature for the color selection controls to appear in the state the user left them
the last time they closed the dialog.




@section api_sec API Reference

See the \ref classes "Classes" and \ref files "Files" tabs for
the full API documentation.

@author David Billen
@version 1.0

**/

/**

@defgroup utilities Utility functions
@defgroup modal Modal functions
@defgroup misc Constants or enums
@defgroup toplevel Top Level Widgets

@page Widgets
@ingroup toplevel
- cosel::rgb_selector: Sliders for setting a color in RGB
- cosel::hsv_selector: Sliders for setting a color in HSV (or HSV+)
- cosel::hsl_selector: Sliders for setting a color in HSL
- cosel::hsl2_selector: Sliders for setting a color in HSL+
- cosel::ring_selector: 2D Ring for setting a color in HSV
- cosel::twod_selector: 2D Grid for setting a color in HSV
- cosel::alpha_widget: Single slider for setting an alpha value
- cosel::html_widget: Edit box for entering an sRGB hex color
- cosel::info_widget: Advanced editor for manually entering a color using various models.
- cosel::color_swatch: Displays a color supporting drag-and-drop and alpha


**/
