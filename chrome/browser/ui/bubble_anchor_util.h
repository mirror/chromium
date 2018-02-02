// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_BUBBLE_ANCHOR_UTIL_H_
#define CHROME_BROWSER_UI_BUBBLE_ANCHOR_UTIL_H_

namespace bubble_anchor_util {

// Offset from the window edge to show bubbles when there is no location bar.
// E.g., when in fullscreen or in a Hosted App window. Don't center, since that
// could obscure a fullscreen bubble.
constexpr int kNoToolbarLeftOffset = 40;

}  // namespace bubble_anchor_util

#endif  // CHROME_BROWSER_UI_BUBBLE_ANCHOR_UTIL_H_
