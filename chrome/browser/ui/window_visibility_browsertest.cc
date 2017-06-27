// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/window_visibility.h"

#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/test/base/in_process_browser_test.h"

// GetWindowVisibilityMap is only implemented on ash.
#if defined(USE_ASH)

namespace {

class WindowVisibilityBrowserTest : public InProcessBrowserTest {};

}  // namespace

// Verify that a window which is completely covered by another window is not
// considered visible.
IN_PROC_BROWSER_TEST_F(WindowVisibilityBrowserTest, WindowOnTopOfOtherWindow) {
  Browser* browser1 = BrowserList::GetInstance()->GetLastActive();
  browser1->window()->SetBounds(gfx::Rect(10, 10, 10, 10));

  chrome::NewWindow(browser1);
  Browser* browser2 = BrowserList::GetInstance()->GetLastActive();
  EXPECT_NE(browser1, browser2);
  browser2->window()->SetBounds(gfx::Rect(0, 0, 100, 100));

  ui::WindowVisibilityMap window_visibility_map = ui::GetWindowVisibilityMap();
  EXPECT_FALSE(window_visibility_map[browser1->window()->GetNativeWindow()]);
  EXPECT_TRUE(window_visibility_map[browser2->window()->GetNativeWindow()]);
}

// Verify that a minimized window is not considered visible.
IN_PROC_BROWSER_TEST_F(WindowVisibilityBrowserTest, MinimizedWindow) {
  Browser* browser = BrowserList::GetInstance()->GetLastActive();
  browser->window()->Minimize();
  ui::WindowVisibilityMap window_visibility_map = ui::GetWindowVisibilityMap();
  EXPECT_FALSE(window_visibility_map[browser->window()->GetNativeWindow()]);
}

// Verify that visibility is computed correctly when multiple windows are open.
IN_PROC_BROWSER_TEST_F(WindowVisibilityBrowserTest, MultipleWindows) {
  // |browser2| covers |browser1|. |browser3| is visible and does not cover
  // another browser. |browser4| is minimized. Its bounds would cover all other
  // browsers if it wasn't minimized.
  Browser* browser1 = BrowserList::GetInstance()->GetLastActive();
  browser1->window()->SetBounds(gfx::Rect(10, 10, 10, 10));

  chrome::NewWindow(browser1);
  Browser* browser2 = BrowserList::GetInstance()->GetLastActive();
  EXPECT_NE(browser1, browser2);
  browser2->window()->SetBounds(gfx::Rect(0, 0, 100, 100));

  chrome::NewWindow(browser1);
  Browser* browser3 = BrowserList::GetInstance()->GetLastActive();
  EXPECT_NE(browser1, browser3);
  EXPECT_NE(browser2, browser3);
  browser3->window()->SetBounds(gfx::Rect(150, 0, 100, 100));

  chrome::NewWindow(browser1);
  Browser* browser4 = BrowserList::GetInstance()->GetLastActive();
  EXPECT_NE(browser1, browser4);
  EXPECT_NE(browser2, browser4);
  EXPECT_NE(browser3, browser4);
  browser4->window()->SetBounds(gfx::Rect(0, 0, 300, 300));
  browser4->window()->Minimize();

  ui::WindowVisibilityMap window_visibility_map = ui::GetWindowVisibilityMap();
  EXPECT_FALSE(window_visibility_map[browser1->window()->GetNativeWindow()]);
  EXPECT_TRUE(window_visibility_map[browser2->window()->GetNativeWindow()]);
  EXPECT_TRUE(window_visibility_map[browser3->window()->GetNativeWindow()]);
  EXPECT_FALSE(window_visibility_map[browser4->window()->GetNativeWindow()]);
}

#endif  // defined(USE_ASH)
