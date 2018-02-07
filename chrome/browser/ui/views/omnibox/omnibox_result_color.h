// Copyright (c) 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_OMNIBOX_OMNIBOX_RESULT_COLOR_H_
#define CHROME_BROWSER_UI_VIEWS_OMNIBOX_OMNIBOX_RESULT_COLOR_H_

#include "base/macros.h"
#include "third_party/skia/include/core/SkColor.h"

namespace views {
class View;
}  // namespace views

class OmniboxResultColor {
 public:
  // Keep these ordered from least dominant (normal) to most dominant
  // (selected).
  enum HighlightState { NORMAL = 0, HOVERED, SELECTED, NUM_STATES };

  enum ColorKind {
    BACKGROUND = 0,
    TEXT,
    DIMMED_TEXT,
    URL,
    INVISIBLE_TEXT,
    NUM_KINDS
  };

  static SkColor GetColor(HighlightState state,
                          ColorKind kind,
                          const views::View* view);
};

#endif  // CHROME_BROWSER_UI_VIEWS_OMNIBOX_OMNIBOX_RESULT_COLOR_H_
