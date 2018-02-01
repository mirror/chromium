// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/browser_window_views_mac.h"

#include "build/buildflag.h"
#import "chrome/browser/ui/cocoa/browser_window_controller.h"
#import "chrome/browser/ui/cocoa/tabs/tab_window_controller.h"
#include "chrome/browser/ui/views_mode_controller.h"
#include "ui/base/ui_features.h"

namespace {

bool HasCocoaWindow() {
#if BUILDFLAG(MAC_VIEWS_BROWSER)
  return views_mode_controller::IsViewsBrowserCocoa();
#else
  return true;
#endif
}

}  // namespace

BrowserWindowController* BrowserWindowControllerForWindow(NSWindow* window) {
  return HasCocoaWindow()
             ? [BrowserWindowController browserWindowControllerForWindow:window]
             : nullptr;
}

TabWindowController* TabWindowControllerForWindow(NSWindow* window) {
  return HasCocoaWindow()
             ? [TabWindowController tabWindowControllerForWindow:window]
             : nullptr;
}
