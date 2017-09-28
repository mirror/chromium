// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CLEAN_CHROME_BROWSER_UI_MAIN_CONTENT_MAIN_CONTENT_VIEW_CONTROLLER_SUBCLASSING_H_
#define IOS_CLEAN_CHROME_BROWSER_UI_MAIN_CONTENT_MAIN_CONTENT_VIEW_CONTROLLER_SUBCLASSING_H_

// MainContentViewController category used by subclasses.
@interface MainContentViewController (MainContentSubclassing)
// UIScrollView properties that can be set from the subclass.  The superclass
// converts these into broadcasted values.
@property(nonatomic) CGPoint contentOffset;
@property(nonatomic, getter=isDragging) BOOL dragging;
@property(nonatomic, getter=isDecelerating) BOOL decelerating;
@end

#endif  // IOS_CLEAN_CHROME_BROWSER_UI_MAIN_CONTENT_MAIN_CONTENT_VIEW_CONTROLLER_SUBCLASSING_H_
