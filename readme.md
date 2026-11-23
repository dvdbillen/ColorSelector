# Cosel

Cosel is a set of color selection classes for Qt that can be used independently, within a customizable widget, or as a drop-in replacement for `QColorDialog`.

Cosel is color-science aware, allowing for greater functionality than purely RGB-based selectors.

#### Screen Shots
##### Default look with dark style
<img width="802" height="582" alt="Screenshot 2026-11-23 084529" src="https://github.com/user-attachments/assets/2c52a93c-0792-4348-a0fd-060e862e206d" />

##### Add full corner rounding and alpha selector using light style
<img width="913" height="693" alt="Screenshot 2026-11-23 085225" src="https://github.com/user-attachments/assets/8652afba-a0c6-4335-837f-2bc0cb8fa957" />

##### No corner rounding and advanced edit tab
<img width="802" height="567" alt="Screenshot 2026-11-23 085535" src="https://github.com/user-attachments/assets/ddd33c08-04cb-4240-97ca-6d47c6ed95fa" />

### Features

- All `QColorDialog` features (screen picker, custom palette, HTML entry, etc.)

- Stand-alone components or full `QColorDialog` replacement

- Sliders with dynamic gradients

- Perceptually uniform hue selection

- Customizable look (corner rounding, slider thickness, pointer orientation, etc.)

- RGB, HSV, and HSL slider tabs (all optional)

- Unique HSL+ and HSV+ slider tabs (optional)

- 2D HSV ring selector (optional)

- 2D HSV+ color grid selector (optional)

- Drag-and-drop support for custom palettes, etc.

### Adding Cosel to your application
##### Inclusion
Cosel is header-only code. Adding to an existing Qt project is simply a matter of including the headers. 
##### Dependencies
Cosel is dependent on Qt version 6.6 (or later) and C++ 17 (or later). The demo app depends on C++ 20.
##### Compiler settings
The only known requirement for the compiler is to avoid options that sacrifice floating point precision for performance, such as **-ffast-math** on GNU compatible compilers.


---

## Using the Qt Compatible Dialog
Here's an overview of replacing the Qt dialog. For more details see the demo application or generate the Doxygen documentation.

```
#include "color_select/cosel_qt_dialog.h"

QColor choose_color(const QColor current_color) {
	// Just replace QColorDialog with cosel::color_dialog
	cosel::color_dialog dialog(current, this);

	// All options are supported
    dialog.setOption(QColorDialog::ShowAlphaChannel, true);

	// Execute the dialog
    if(dialog.exec() == QDialog::Accepted) {
         return dialog.selectedColor();
    }
    
    return current_color;
}

```


You can set additional cosel options as well:


```
#include "color_select/cosel_qt_dialog.h"

QColor choose_color(const QColor current_color) {
	// Replace QColorDialog with cosel::color_dialog
	cosel::color_dialog dialog(current, this);

	// Set some options using the Qt interface 
    dialog.setOption(QColorDialog::ShowAlphaChannel, true);
    
    // Set some cosel specifc options also
    
    // Copy existing settings (if you need to avoid writing over Qt settings)
    auto extra_options = dialog.cosel_options();
    
    // Modify them
    extra_options.slider_style.rounding = 1.0;  // Sausage sliders
    extra_options.show_2d_edit_tab = true;      // Show advanced edit tab
    
    // Set the modified options
    dialog.set_cosel_options(extra_options);
    

	// Execute the dialog
    if(dialog.exec() == QDialog::Accepted) {
         return dialog.selectedColor();
    }
    
    return current_color;
}

```

## Using individual components
Most of the color selection widgets inside the dialog can be used in an application individually. These are documented in the Doxygen documentation, (which must be generated).

---

## HSL+ and HSV+
The cosel selector supports two non-standard color models: HSL+ and HSV+. They are not enabled by default.

#### HSL+
The HSL+ sliders work much like conventional HSL sliders but give a consistent perceptual brightness level at all hue and saturation levels.

##### Detailed Description
The 'L' slider specifies an exact luminance that matches 'L' in CIE LAB/LUV. This allows selection of colors with exact contrast. For example a color used for a background with L 15 and a color used for a foreground with L 68 will have the same perceptual contrast regardless of the hue or saturation. 

Pure colors have a limited luminance. To accommodate this fact, the S level ranges from gray at zero to the most saturation that the specified hue can achieve at the specified L. Just as with HSL, when L is full (100), the only color that can be selected is white.

None of the HSL+ slider values are consistent with conventional HSL.


#### HSV+
The HSV+ sliders work much like conventional HSV sliders but give a more consistent perceptual saturation level for different hues.

##### Detailed Description
In conventional HSV the 'S' slider determines a direct percentage of gray in the final RGB color. The visible level of saturation varies depending on the selected hue. For example, blue looks less saturated than green at the same S value. 

In HSV+ the 'H' and 'V' sliders are compatible with HSV, but the 'S' slider determines the amount of white using the CIE LUV color model. This gives it a better consistency for all hues.


---
### Advanced Applications
Cosel does not hard-code the color space. Currently it can be built with several that are included and accommodates easily creating new ones.

However, at this time there is no support for dynamically selectable color spaces, as it adds much to the code injecting the color space into function arguments and I can't predict that there would be a lot of interest in this capability (which would likely also include profile handling).

If you are someone with interest in using Cosel in an application that supports selection of a color-space then contact me.
