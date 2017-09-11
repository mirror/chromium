// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CLEAN_CHROME_BROWSER_UI_TOOLBAR_TOOLBAR_BUTTON_FACTORY_NORMAL_H_
#define IOS_CLEAN_CHROME_BROWSER_UI_TOOLBAR_TOOLBAR_BUTTON_FACTORY_NORMAL_H_

#import <UIKit/UIKit.h>

#import "ios/clean/chrome/browser/ui/toolbar/toolbar_button_factory.h"

// Implementation of the ToolbarButtonFactory, creating button with the normal
// (non-incognito) style.
@interface ToolbarButtonFactoryNormal : NSObject<ToolbarButtonFactory>
@end

#endif  // IOS_CLEAN_CHROME_BROWSER_UI_TOOLBAR_TOOLBAR_BUTTON_FACTORY_NORMAL_H_
