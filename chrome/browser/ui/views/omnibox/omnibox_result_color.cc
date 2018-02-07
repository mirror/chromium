// Copyright (c) 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <algorithm>  // NOLINT

#include "base/macros.h"
#include "chrome/browser/ui/views/omnibox/omnibox_result_color.h"

#include "chrome/grit/generated_resources.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/gfx/color_palette.h"
#include "ui/gfx/color_utils.h"
#include "ui/native_theme/native_theme.h"
#include "ui/views/view.h"

using ui::NativeTheme;

namespace {

// A mapping from HighlightState/ColorKind types to NativeTheme colors.
struct TranslationTable {
  ui::NativeTheme::ColorId id;
  OmniboxResultColor::HighlightState state;
  OmniboxResultColor::ColorKind kind;
} static const kTranslationTable[] = {
    {NativeTheme::kColorId_ResultsTableNormalBackground,
     OmniboxResultColor::NORMAL, OmniboxResultColor::BACKGROUND},
    {NativeTheme::kColorId_ResultsTableHoveredBackground,
     OmniboxResultColor::HOVERED, OmniboxResultColor::BACKGROUND},
    {NativeTheme::kColorId_ResultsTableSelectedBackground,
     OmniboxResultColor::SELECTED, OmniboxResultColor::BACKGROUND},
    {NativeTheme::kColorId_ResultsTableNormalText, OmniboxResultColor::NORMAL,
     OmniboxResultColor::TEXT},
    {NativeTheme::kColorId_ResultsTableHoveredText, OmniboxResultColor::HOVERED,
     OmniboxResultColor::TEXT},
    {NativeTheme::kColorId_ResultsTableSelectedText,
     OmniboxResultColor::SELECTED, OmniboxResultColor::TEXT},
    {NativeTheme::kColorId_ResultsTableNormalDimmedText,
     OmniboxResultColor::NORMAL, OmniboxResultColor::DIMMED_TEXT},
    {NativeTheme::kColorId_ResultsTableHoveredDimmedText,
     OmniboxResultColor::HOVERED, OmniboxResultColor::DIMMED_TEXT},
    {NativeTheme::kColorId_ResultsTableSelectedDimmedText,
     OmniboxResultColor::SELECTED, OmniboxResultColor::DIMMED_TEXT},
    {NativeTheme::kColorId_ResultsTableNormalUrl, OmniboxResultColor::NORMAL,
     OmniboxResultColor::URL},
    {NativeTheme::kColorId_ResultsTableHoveredUrl, OmniboxResultColor::HOVERED,
     OmniboxResultColor::URL},
    {NativeTheme::kColorId_ResultsTableSelectedUrl,
     OmniboxResultColor::SELECTED, OmniboxResultColor::URL},
};

}  // namespace

SkColor OmniboxResultColor::GetColor(HighlightState state,
                                     ColorKind kind,
                                     const views::View* view) {
  if (kind == INVISIBLE_TEXT)
    return SK_ColorTRANSPARENT;
  for (size_t i = 0; i < arraysize(kTranslationTable); ++i) {
    if (kTranslationTable[i].state == state &&
        kTranslationTable[i].kind == kind) {
      return view->GetNativeTheme()->GetSystemColor(kTranslationTable[i].id);
    }
  }

  NOTREACHED();
  return gfx::kPlaceholderColor;
}
