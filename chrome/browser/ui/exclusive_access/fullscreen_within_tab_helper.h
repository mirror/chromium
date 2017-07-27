// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_EXCLUSIVE_ACCESS_FULLSCREEN_WITHIN_TAB_HELPER_H_
#define CHROME_BROWSER_UI_EXCLUSIVE_ACCESS_FULLSCREEN_WITHIN_TAB_HELPER_H_

#include "base/macros.h"
#include "build/build_config.h"
#include "content/public/browser/web_contents_user_data.h"
#if defined(OS_MACOSX)
#include "ui/gfx/native_widget_types.h"
#endif

// Helper used by FullscreenController to track the state of a WebContents that
// is in fullscreen mode, but the browser window is not. See
// 'FullscreenWithinTab Note' in fullscreen_controller.h.
//
// The purpose of this class is to associate some fullscreen state at the tab
// level rather than at the Browser level. This allows tabs to be
// dragged/dropped between Browsers and have their fullscreen state handed off
// between FullscreenControllers as well.
//
// FullscreenWithinTabHelper is created on-demand, and its lifecycle is tied to
// that of its associated WebContents. It is automatically destroyed.
class FullscreenWithinTabHelper
    : public content::WebContentsUserData<FullscreenWithinTabHelper> {
 public:
  ~FullscreenWithinTabHelper() override;

#if defined(OS_MACOSX)
  bool is_fullscreen_or_pending() const { return is_fullscreen_or_pending_; }

  void SetIsFullscreenOrPending(bool is_fullscreen) {
    is_fullscreen_or_pending_ = is_fullscreen;
  }

  gfx::NativeWindow separate_fullscreen_window() {
    return separate_fullscreen_window_;
  }

  void SetSeparateFullscreenWindow(gfx::NativeWindow window) {
    separate_fullscreen_window_ = window;
  }
#endif

  bool is_fullscreen_for_captured_tab() const {
    return is_fullscreen_for_captured_tab_;
  }

  void SetIsFullscreenForCapturedTab(bool is_fullscreen) {
    is_fullscreen_for_captured_tab_ = is_fullscreen;
  }

  // Immediately remove and destroy the FullscreenWithinTabHelper instance
  // associated with |web_contents|.
  static void RemoveForWebContents(content::WebContents* web_contents);

 private:
  friend class content::WebContentsUserData<FullscreenWithinTabHelper>;
  explicit FullscreenWithinTabHelper(content::WebContents* ignored);

#if defined(OS_MACOSX)
  // Set to true if the contents is in fullscreen or in the process of being
  // fullscreen.
  bool is_fullscreen_or_pending_ = false;
  // Reference to the fullscreen window created to display the WebContents
  // view separately.
  gfx::NativeWindow separate_fullscreen_window_ = nullptr;
#endif

  bool is_fullscreen_for_captured_tab_;

  DISALLOW_COPY_AND_ASSIGN(FullscreenWithinTabHelper);
};

#endif  // CHROME_BROWSER_UI_EXCLUSIVE_ACCESS_FULLSCREEN_WITHIN_TAB_HELPER_H_
