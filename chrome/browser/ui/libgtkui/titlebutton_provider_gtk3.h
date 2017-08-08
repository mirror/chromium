// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_LIBGTKUI_TITLEBUTTON_PROVIDER_GTK3_H_
#define CHROME_BROWSER_UI_LIBGTKUI_TITLEBUTTON_PROVIDER_GTK3_H_

#include "chrome/browser/ui/libgtkui/libgtkui_export.h"
#include "ui/gfx/image/image_skia.h"
#include "ui/views/controls/button/button.h"

namespace libgtkui {

// TODO(thomasanderson): change |type| to a real type
LIBGTKUI_EXPORT gfx::ImageSkia GetTitlebuttonImage(
    int type,
    views::Button::ButtonState state,
    int available_height);
}  // namespace libgtkui

#endif  // CHROME_BROWSER_UI_LIBGTKUI_TITLEBUTTON_PROVIDER_GTK3_H_
