// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_HIGHLIGHTER_HIGHLIGHTER_DELEGATE_H_
#define ASH_HIGHLIGHTER_HIGHLIGHTER_DELEGATE_H_

namespace gfx {
class Rect;
}  // namespace gfx

namespace ash {

// Delegate for taking screenshots.
class HighlighterDelegate {
 public:
  virtual ~HighlighterDelegate() {}

  virtual void HandleSelection(const gfx::Rect& rect) = 0;
};
}  // namespace ash

#endif  // ASH_HIGHLIGHTER_HIGHLIGHTER_DELEGATE_H_
