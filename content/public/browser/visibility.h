// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_BROWSER_VISIBILITY_H_
#define CONTENT_PUBLIC_BROWSER_VISIBILITY_H_

namespace content {

enum class Visibility {
  // The view is visible.
  VISIBLE,
  // The view is obscured by other views.
  OCCLUDED,
  // The view is hidden (e.g. part of a minimized window or not part of any
  // window).
  HIDDEN,
  // Note: A view can be captured (e.g. for screenshots or mirroring) even if
  // it's not VISIBLE.
  // Note: The Visibility of an occluded view that doesn't support occlusion
  // tracking is VISIBLE.
};

}  // namespace content

#endif  // CONTENT_PUBLIC_BROWSER_VISIBILITY_H_
