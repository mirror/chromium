// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_TABS_TAB_SPINNER_ICON_VIEW_H_
#define CHROME_BROWSER_UI_COCOA_TABS_TAB_SPINNER_ICON_VIEW_H_

#import "chrome/browser/ui/cocoa/spinner_view.h"

#import "chrome/browser/ui/cocoa/tabs/tab_controller.h"

@interface TabSpinnerIconView : SpinnerView {
  TabLoadingState tabLoadingState_;
}

@property(assign, nonatomic) TabLoadingState tabLoadingState;

- (void)setTabDoneIcon:(NSImage*)anImage;

@end

#endif  // CHROME_BROWSER_UI_COCOA_TABS_TAB_SPINNER_ICON_VIEW_H_
