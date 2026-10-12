# Cosel

> **Status: Early / WIP**  

> This project is functional but not yet ready for general use.  

> It is largely undocumented, the included demo is only a temporary placeholder, and it has so far only been tested on Windows (not yet on Linux or macOS).  

> Expect the public interface and demo to change.



## Overview

Cosel is a set of color selection classes for Qt that can be used independently, within a customizable widget, or as a drop-in replacement for `QColorDialog`.



Cosel is color-science aware, allowing for greater functionality than purely RGB-based selectors.



## Dependencies

- Qt 6.x

- C++17 or higher (demo requires C++20)



## Features

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



