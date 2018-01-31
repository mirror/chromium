// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/chromeos/search_box/search_box_constants.h"

namespace search_box {

// Default color used when wallpaper customized color is not available for
// searchbox, #000 at 87% opacity.
const SkColor kDefaultSearchboxColor =
    SkColorSetARGBMacro(0xDE, 0x00, 0x00, 0x00);

// The horizontal padding of the box layout of the search box.
const int kPadding = 12;

// The default background color of the search box.
const SkColor kSearchBoxBackgroundDefault = SK_ColorWHITE;

// The background border corner radius of the search box.
const int kSearchBoxBorderCornerRadius = 24;

// Preferred height of search box.
const int kSearchBoxPreferredHeight = 48;

// The size of the search icon in the search box.
const int kSearchIconSize = 24;

}  // namespace search_box
