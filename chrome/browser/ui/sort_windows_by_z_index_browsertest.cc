// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/sort_windows_by_z_index.h"

#include <vector>

#include "base/containers/flat_set.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/test/base/in_process_browser_test.h"

// SortWindowsByZIndex is only implemented on ash.
#if defined(USE_ASH)

namespace {

class SortWindowsByZIndexBrowserTest : public InProcessBrowserTest {};

}  // namespace

// Verify that visibility is computed correctly when multiple windows are open.
IN_PROC_BROWSER_TEST_F(SortWindowsByZIndexBrowserTest, SortWindowsByZIndex) {
  Browser* browser1 = BrowserList::GetInstance()->GetLastActive();
  chrome::NewWindow(browser1);
  Browser* browser2 = BrowserList::GetInstance()->GetLastActive();
  EXPECT_NE(browser1, browser2);
  chrome::NewWindow(browser1);
  Browser* browser3 = BrowserList::GetInstance()->GetLastActive();
  EXPECT_NE(browser1, browser3);
  EXPECT_NE(browser2, browser3);
  chrome::NewWindow(browser1);
  Browser* browser4 = BrowserList::GetInstance()->GetLastActive();
  EXPECT_NE(browser1, browser4);
  EXPECT_NE(browser2, browser4);
  EXPECT_NE(browser3, browser4);

  gfx::NativeWindow window1 = browser1->window()->GetNativeWindow();
  gfx::NativeWindow window2 = browser2->window()->GetNativeWindow();
  gfx::NativeWindow window3 = browser3->window()->GetNativeWindow();
  gfx::NativeWindow window4 = browser4->window()->GetNativeWindow();

  base::flat_set<gfx::NativeWindow> windows(
      {window1, window3, window2, window4}, base::KEEP_FIRST_OF_DUPES);
  const std::vector<gfx::NativeWindow> sorted_windows =
      ui::SortWindowsByZIndex(windows);
  const std::vector<gfx::NativeWindow> expected_sorted_windows(
      {window4, window3, window2, window1});
  EXPECT_EQ(expected_sorted_windows, sorted_windows);
}

#endif  // defined(USE_ASH)
