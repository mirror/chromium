// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_BROWSER_VISIBILITY_H_
#define CONTENT_PUBLIC_BROWSER_VISIBILITY_H_

namespace content {

enum class Visibility {
  // The view is visible.
  VISIBLE,
  // The view is fully obscured by other views/windows.
  OCCLUDED,
  // The view is hidden (e.g. belongs to a minimized window, doesn't belong to a
  // window).
  HIDDEN,
  // Note: A view can be captured (e.g. for screenshots or mirroring) even if
  // it's not VISIBLE.
  // Note: The Visibility of an occluded view can be VISIBLE if it doesn't have
  // full occlusion tracking support.
};

}  // namespace content

#endif  // CONTENT_PUBLIC_BROWSER_VISIBILITY_H_
