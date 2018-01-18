// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_STATUS_BUBBLE_MAC_TESTING_H_
#define CHROME_BROWSER_UI_COCOA_STATUS_BUBBLE_MAC_TESTING_H_

#import "chrome/browser/ui/cocoa/status_bubble_mac.h"

// StatusBubbleWindow becomes a child of |statusBubbleParentWindow|, but waits
// until |statusBubbleParentWindow| is visible. This works around macOS
// bugs/features which make unexpected things happen when adding a child window
// to a window that's in another space, miniaturized, or hidden
// (https://crbug.com/783521, https://crbug.com/798792).
@interface StatusBubbleWindow : NSWindow
@property(assign, nonatomic) NSWindow* statusBubbleParentWindow;
@end

#endif  // CHROME_BROWSER_UI_COCOA_STATUS_BUBBLE_MAC_TESTING_H_
