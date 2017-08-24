// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_VR_VR_URL_UTIL_H_
#define CHROME_BROWSER_VR_VR_URL_UTIL_H_

#include "base/strings/string16.h"
#include "chrome/browser/vr/ui_unsupported_mode.h"
#include "third_party/skia/include/core/SkColor.h"
#include "url/gurl.h"

namespace gfx {
class RenderText;
}  // namespace gfx

namespace vr {

bool RenderUrlHelper(const base::string16& formatted_url,
                     const url::Parsed& parsed,
                     SkColor default_text_color,
                     int pixel_font_height,
                     gfx::RenderText* rendered_text,
                     UiUnsupportedMode* failure_reason);

}  // namespace vr

#endif  // CHROME_BROWSER_VR_VR_URL_UTIL_H_
