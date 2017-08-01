// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_CONTENT_WIDGET_EXTENSION_CONTENT_WIDGET_VIEW_H_
#define IOS_CHROME_CONTENT_WIDGET_EXTENSION_CONTENT_WIDGET_VIEW_H_

#import <UIKit/UIKit.h>

// View for the content widget. Shows 1 (compact)or 2 (expanded) rows of 4 most
// visited tiles (favicon or fallback + title).
@interface ContentWidgetView : UIView

// The height of the widget in expanded mode.
@property(nonatomic, readonly) CGFloat widgetExpandedHeight;

// Designated initializer, creates the widget view. |compactHeight| indicates
// the size to use in compact display. |initiallyCompact| indicates which mode
// to display on initialization.
- (instancetype)initWithCompactHeight:(CGFloat)compactHeight
                     initiallyCompact:(BOOL)compact NS_DESIGNATED_INITIALIZER;
- (instancetype)initWithFrame:(CGRect)frame NS_UNAVAILABLE;
- (instancetype)initWithCoder:(NSCoder*)aDecoder NS_UNAVAILABLE;
- (instancetype)init NS_UNAVAILABLE;

// Changes the display mode of the view to compact or expanded.
- (void)showMode:(BOOL)compact;

@end

#endif  // IOS_CHROME_CONTENT_WIDGET_EXTENSION_CONTENT_WIDGET_VIEW_H_
