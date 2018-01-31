// Copyright (c) 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_VIEWS_LAYOUT_LAYOUT_CHANGES_H_
#define UI_VIEWS_LAYOUT_LAYOUT_CHANGES_H_

#include <utility>
#include <vector>

namespace gfx {
class Rect;
}

namespace views {

class View;

using LayoutChanges = std::vector<std::pair<View*, gfx::Rect>>;

}  // namespace views

#endif  // UI_VIEWS_LAYOUT_LAYOUT_CHANGES_H_
