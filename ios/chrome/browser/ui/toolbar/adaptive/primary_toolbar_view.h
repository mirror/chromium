// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_TOOLBAR_ADAPTIVE_PRIMARY_TOOLBAR_VIEW_H_
#define IOS_CHROME_BROWSER_UI_TOOLBAR_ADAPTIVE_PRIMARY_TOOLBAR_VIEW_H_

#import <UIKit/UIKit.h>

#import "ios/chrome/browser/ui/toolbar/adaptive/adaptive_toolbar_view.h"

// View for the primary toolbar. In an adaptive toolbar paradigm, this is the
// toolbar always presented.
@interface PrimaryToolbarView : UIView<AdaptiveToolbarView>

@end

#endif  // IOS_CHROME_BROWSER_UI_TOOLBAR_ADAPTIVE_PRIMARY_TOOLBAR_VIEW_H_
