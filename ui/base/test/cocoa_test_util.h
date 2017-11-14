// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_BASE_TEST_COCOA_TEST_UTIL_H_
#define UI_BASE_TEST_COCOA_TEST_UTIL_H_

#include "base/optional.h"

@class NSView;

namespace ui {

struct ViewTreeProblemDetails {
  enum ProblemType {
    // |view_a| (the child view) is outside |view_b| (the parent view).
    VIEW_OUTSIDE_PARENT,

    // |view_a| and |view_b|, neither of which is an ancestor of the other,
    // overlap each other.
    VIEWS_OVERLAP,
  };

  ProblemType type;
  NSView* view_a;
  NSView* view_b;

  std::string ToString();
};

// Validates the view tree rooted at |root|. If at least one problem is found,
// returns a |ViewTreeProblemDetails| as described above; if not, returns an
// empty option.
base::Optional<ViewTreeProblemDetails> ValidateViewTree(NSView* root);

}  // namespace ui

#endif  // UI_BASE_TEST_COCOA_TEST_UTIL_H_
