// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_LIBGTKUI_TITLEBUTTON_PROVIDER_GTK3_H_
#define CHROME_BROWSER_UI_LIBGTKUI_TITLEBUTTON_PROVIDER_GTK3_H_

#include "chrome/browser/ui/libgtkui/libgtkui_export.h"
#include "ui/gfx/image/image_skia.h"
#include "ui/views/controls/button/button.h"
#include "ui/views/window/frame_buttons.h"

namespace libgtkui {

LIBGTKUI_EXPORT gfx::ImageSkia GetTitlebuttonImage(
    views::FrameButtonDisplayType type,
    views::Button::ButtonState state,
    int top_area_height);

LIBGTKUI_EXPORT int GetWindowCaptionSpacing(int top_area_height);

}  // namespace libgtkui

#endif  // CHROME_BROWSER_UI_LIBGTKUI_TITLEBUTTON_PROVIDER_GTK3_H_
