// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_VIEWS_WORD_LOOKUP_CLIENT_H_
#define UI_VIEWS_WORD_LOOKUP_CLIENT_H_

#include "ui/views/views_export.h"

namespace gfx {
struct DecoratedText;
class Point;
class Rect;
}

namespace views {

// An interface implemented by a view which supports word lookups.
class VIEWS_EXPORT WordLookupClient {
 public:
  // Retrieves the word displayed at the given |point| along with its styling
  // information. |point| is in the coordinate system of the view. If no word is
  // displayed at the point, returns a nearby word. |baseline_point| should
  // correspond to the baseline point of the leftmost glyph of the |word| in the
  // view's coordinates. Returns false, if no word can be retrieved.
  virtual bool GetDecoratedWordAtPoint(const gfx::Point& point,
                                       gfx::DecoratedText* decorated_word,
                                       gfx::Point* baseline_point) = 0;

  // Returns the bounds of the text in the given |range| in the coordinate
  // system of the view.
  virtual gfx::Rect GetRelativeBoundsForRange(const gfx::Range& range) = 0;

 protected:
  virtual ~WordLookupClient() {}
};

}  // namespace views

#endif  // UI_VIEWS_WORD_LOOKUP_CLIENT_H_
