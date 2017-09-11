// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/clean/chrome/browser/ui/toolbar/toolbar_configuration_normal.h"

#import "ios/chrome/browser/ui/uikit_ui_util.h"
#import "ios/clean/chrome/browser/ui/toolbar/toolbar_constants.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@implementation ToolbarConfigurationNormal

- (UIColor*)backgroundColor {
  return UIColorFromRGB(kToolbarBackgroundColor);
}

- (UIColor*)omniboxBackgroundColor {
  return [UIColor whiteColor];
}

- (UIColor*)omniboxBorderColor {
  return UIColorFromRGB(kIncognitoLocationBarBorderColor);
}

@end
