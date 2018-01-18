// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_VR_ELEMENTS_OMNIBOX_FORMATTING_H_
#define CHROME_BROWSER_VR_ELEMENTS_OMNIBOX_FORMATTING_H_

#include <memory>

#include "chrome/browser/vr/elements/text.h"
#include "chrome/browser/vr/model/color_scheme.h"
#include "components/omnibox/browser/autocomplete_match.h"

namespace gfx {
class RenderText;
}

namespace vr {

// The set of URL format attributes used by VR.
extern const url_formatter::FormatUrlTypes kUrlFormatTypes;

// Convert Autocomplete's suggestion formatting to generic VR text formatting.
TextFormatting ConvertClassification(
    const ACMatchClassifications& classifications,
    size_t text_length,
    const ColorScheme& color_scheme);

struct ElisionParameters {
  int offset = 0;
  bool fade_left = false;
  bool fade_right = false;
};

// Based on a URL and the RenderText that will draw it, determine the required
// elision parameters.  This means computing an offset such that the rightmost
// portion of the TLD is visible (along with a small part of the path), and
// fading either edge if they overflow available space.
ElisionParameters GetElisionParameters(const GURL& gurl,
                                       const url::Parsed& parsed,
                                       gfx::RenderText* render_text,
                                       int min_path_pixels);

}  // namespace vr

#endif  // CHROME_BROWSER_VR_ELEMENTS_OMNIBOX_FORMATTING_H_
