// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/vr/vr_url_util.h"

#include "chrome/browser/vr/elements/ui_texture.h"
#include "ui/gfx/font_list.h"
#include "ui/gfx/geometry/rect_f.h"
#include "ui/gfx/render_text.h"

namespace vr {

bool RenderUrlHelper(const base::string16& formatted_url,
                     const url::Parsed& parsed,
                     SkColor default_text_color,
                     int pixel_font_height,
                     gfx::RenderText* rendered_text,
                     UiUnsupportedMode* failure_reason) {
  DCHECK(rendered_text);

  bool succeed = true;

  gfx::FontList font_list;
  if (!UiTexture::GetFontList(pixel_font_height, formatted_url, &font_list)) {
    *failure_reason = UiUnsupportedMode::kUnhandledCodePoint;
    succeed = false;
  }

  rendered_text->SetFontList(font_list);
  rendered_text->SetColor(default_text_color);
  rendered_text->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  rendered_text->SetElideBehavior(gfx::ELIDE_TAIL);
  rendered_text->SetDirectionalityMode(gfx::DIRECTIONALITY_FORCE_LTR);
  rendered_text->SetText(formatted_url);

  // Until we can properly elide a URL, we need to bail if the origin portion
  // cannot be displayed in its entirety.
  base::string16 mandatory_prefix = formatted_url;
  int length = parsed.CountCharactersBefore(url::Parsed::PORT, false);
  if (length > 0)
    mandatory_prefix = formatted_url.substr(0, length);
  // Ellipsis-based eliding replaces the last character in the string with an
  // ellipsis, so to reliably check that the origin is intact, check both length
  // and string equality.
  if (rendered_text->GetDisplayText().size() < mandatory_prefix.size() ||
      rendered_text->GetDisplayText().substr(0, mandatory_prefix.size()) !=
          mandatory_prefix) {
    *failure_reason = UiUnsupportedMode::kCouldNotElideURL;
    succeed = false;
  }
  return succeed;
}

}  // namespace vr
