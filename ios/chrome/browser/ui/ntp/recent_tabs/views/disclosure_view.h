// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_NTP_RECENT_TABS_VIEWS_DISCLOSURE_VIEW_H_
#define IOS_CHROME_BROWSER_UI_NTP_RECENT_TABS_VIEWS_DISCLOSURE_VIEW_H_

#import <UIKit/UIKit.h>

// View indicating whether a table view section is expanded or not.
@interface DisclosureView : UIImageView

// Designated initializer.
- (instancetype)init NS_DESIGNATED_INITIALIZER;

- (instancetype)initWithFrame:(CGRect)frame NS_UNAVAILABLE;

- (instancetype)initWithCoder:(NSCoder*)decoder NS_UNAVAILABLE;

// Sets whether the view indicates that the section is collapsed or not, with an
// animation or not.
- (void)setTransformWhenCollapsed:(BOOL)collapsed animated:(BOOL)animated;

@end

#endif  // IOS_CHROME_BROWSER_UI_NTP_RECENT_TABS_VIEWS_DISCLOSURE_VIEW_H_
