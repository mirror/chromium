// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_HIGHLIGHTER_HIGHLIGHTER_SELECTION_OBSERVER_H_
#define ASH_HIGHLIGHTER_HIGHLIGHTER_SELECTION_OBSERVER_H_

namespace aura {
class Window;
}

namespace gfx {
class Rect;
}

namespace ash {

// Observer for handling highlighter selection.
class HighlighterSelectionObserver {
 public:
  virtual ~HighlighterSelectionObserver() {}

  // |rect| is the selected rectangle, in DIP relative to |root_window|.
  virtual void HandleSelection(aura::Window* root_window,
                               const gfx::Rect& rect) = 0;
};

}  // namespace ash

#endif  // ASH_HIGHLIGHTER_HIGHLIGHTER_SELECTION_OBSERVER_H_
