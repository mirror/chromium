// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_VR_VR_URL_UTIL_H_
#define CHROME_BROWSER_VR_VR_URL_UTIL_H_

#include <array>
#include <string>

#include "base/callback.h"
#include "chrome/browser/vr/ui_unsupported_mode.h"
#include "components/security_state/core/security_state.h"
#include "components/url_formatter/url_formatter.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/gfx/geometry/rect_f.h"
#include "url/gurl.h"

namespace gfx {
class RenderText;
}  // namespace gfx

namespace vr {

class RenderTextWrapper;
struct ColorScheme;

// Gets the color of the security chip on the URL bar.
SkColor GetUrlSecurityChipColor(
    const security_state::SecurityLevel security_level,
    bool offline_page,
    const ColorScheme& color_scheme);

// Uses the given parameters to render url into the given RenderText.
void RenderUrl(const GURL& url,
               const url_formatter::FormatUrlTypes format_types,
               const security_state::SecurityLevel security_level,
               const ColorScheme& color_scheme,
               const base::Callback<void(UiUnsupportedMode)>& failure_callback,
               int pixel_font_height,
               bool apply_styling,
               SkColor text_color,
               gfx::RenderText* rendered_text);

void ApplyUrlStyling(const base::string16& formatted_url,
                     const url::Parsed& parsed,
                     const security_state::SecurityLevel security_level,
                     vr::RenderTextWrapper* render_text,
                     const ColorScheme& color_scheme);

}  // namespace vr

#endif  // CHROME_BROWSER_VR_VR_URL_UTIL_H_
