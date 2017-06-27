// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WINDOW_VISIBILITY_H_
#define CHROME_BROWSER_UI_WINDOW_VISIBILITY_H_

#include "base/containers/flat_map.h"
#include "ui/gfx/native_widget_types.h"

namespace ui {

using WindowVisibilityMap = base::flat_map<gfx::NativeWindow, bool>;

// Returns a map where keys are windows known to Chrome and values are booleans
// indicating whether they might be visible on the screen.
//
// TODO(fdoray): Implement this on all platforms. https://crbug.com/731145
WindowVisibilityMap GetWindowVisibilityMap();

}  // namespace ui

#endif  // CHROME_BROWSER_UI_WINDOW_VISIBILITY_H_
