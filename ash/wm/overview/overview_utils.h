// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_WM_OVERVIEW_OVERVIEW_UTILS_H_
#define ASH_WM_OVERVIEW_OVERVIEW_UTILS_H_

namespace aura {
class Window;
}

namespace ash {

class SplitViewController;

// In split view, we need to check if the |window| can be snapped. If yes, we
// will consider it is a fullscreen/maximized window.
bool IsFullOrMaximizedWindow(aura::Window* window,
                             SplitViewController* split_view_controller);

// Returns true if overview mode should use the new animations.
// TODO(wutao): Remove this function when the old overview mode animations
// become obsolete. See https://crbug.com/801465.
bool IsNewOverviewAnimations();

// Returns true if overview mode should use the new ui.
// TODO(sammiequon): Remove this function when the old overview mode ui becomes
// obsolete. See https://crbug.com/782320.
bool IsNewOverviewUi();

}  // namespace ash

#endif  // ASH_WM_OVERVIEW_OVERVIEW_UTILS_H_
