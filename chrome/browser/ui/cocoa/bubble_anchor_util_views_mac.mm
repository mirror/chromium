// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/bubble_anchor_util_views.h"

namespace bubble_anchor_util {

// This is the stub implementation for a Cocoa browser window for calls coming
// from chrome/browser/ui/views code. GetPageInfoAnchorPoint() should always
// take precedence.
views::View* GetPageInfoAnchorView(Browser* browser) {
  return nullptr;
}

}  // namespace bubble_anchor_util
