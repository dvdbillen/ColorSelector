// SPDX-License-Identifier: MIT
// Copyright (c) 2026 David Billen

#ifndef cosel_cosel_h
#define cosel_cosel_h



// This will be the only header required for color selector for applications, though modules still
// use selective headers to avoid excessive compiles. TODO: Add required headers only

// TODO: Sliders not indicating focus at all

// TODO: Focus is complete shit

// TODO: Could info widget be a 3rd 2D tab?

// TODO: Remove qDebug from non-Qt source modules

// TODO: Be consistent with naming classes name_t and members name_

#include "cosel_qt_dialog.h"
#include "cosel_qt_alphapixmap.h"
#include "cosel_qt_app_defs.h"
#include "cosel_qt_gradpixmap.h"
#include "cosel_qt_gradient_slider.h"
#include "cosel_qt_swatch.h"
#include "cosel_model_hsx.h"
#include "cosel_luv_gamut.h"
#include "cosel_gradient.h"
#include "cosel_qt_2d_widget.h"
#include "cosel_qt_select_widget.h"
#include "cosel_qt_info.h"
#include "cosel_qt_info_widget.h"
#include "cosel_qt_html_widget.h"
#include "cosel_qt_throttled.h"
#include "cosel_qt_palette.h"
#endif // cosel_cosel_h
