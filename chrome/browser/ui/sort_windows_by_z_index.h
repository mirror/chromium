// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_SORT_WINDOWS_BY_Z_INDEX_H_
#define CHROME_BROWSER_UI_SORT_WINDOWS_BY_Z_INDEX_H_

#include <vector>

#include "base/containers/flat_set.h"
#include "ui/gfx/native_widget_types.h"

namespace ui {

// Returns a list with the windows in |windows| sorted by z-index, from topmost
// to bottommost.
//
// TODO(fdoray): Implement this on all platforms. https://crbug.com/731145
std::vector<gfx::NativeWindow> SortWindowsByZIndex(
    const base::flat_set<gfx::NativeWindow>& windows);

}  // namespace ui

#endif  // CHROME_BROWSER_UI_SORT_WINDOWS_BY_Z_INDEX_H_
