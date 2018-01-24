// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/picture_in_picture/picture_in_picture_window_controller.h"

#include "build/build_config.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/overlay/overlay_window.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "content/public/browser/web_contents.h"

class PictureInPictureWindowControllerBrowserTest
    : public InProcessBrowserTest {
 public:
  PictureInPictureWindowControllerBrowserTest() {}

  void SetUpWindowController(content::WebContents* web_contents) {
    pip_window_controller_ =
        PictureInPictureWindowController::GetOrCreateForWebContents(
            web_contents);
  }

  PictureInPictureWindowController* window_controller() {
    return pip_window_controller_;
  }

 private:
  PictureInPictureWindowController* pip_window_controller_;

  DISALLOW_COPY_AND_ASSIGN(PictureInPictureWindowControllerBrowserTest);
};

// Checks the creation of the window controller, as well as basic window
// creation and visibility. The window is currently only implemented in views.
#if defined(OS_WIN) || defined(OS_LINUX)
IN_PROC_BROWSER_TEST_F(PictureInPictureWindowControllerBrowserTest,
                       CreationAndVisibility) {
  content::WebContents* active_web_contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  ASSERT_TRUE(active_web_contents != NULL);

  SetUpWindowController(active_web_contents);
  ASSERT_TRUE(window_controller() != NULL);

  ASSERT_TRUE(window_controller()->GetWindowForTesting() == NULL);
  window_controller()->Init();
  ASSERT_TRUE(window_controller()->GetWindowForTesting() != NULL);
  window_controller()->Show();
  ASSERT_TRUE(window_controller()->GetWindowForTesting()->IsVisible());
}
#endif
