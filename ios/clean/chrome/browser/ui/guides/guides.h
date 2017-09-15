// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CLEAN_CHROME_BROWSER_UI_GUIDES_GUIDES_H_
#define IOS_CLEAN_CHROME_BROWSER_UI_GUIDES_GUIDES_H_

#import <UIKit/UIKit.h>

typedef NSString GuideName;

extern GuideName* const kOmniboxGuide;

UILayoutGuide* FindNamedGuide(GuideName* name, UIView* view);

UILayoutGuide* AddNamedGuide(GuideName* name, UIView* view);

#endif  // IOS_CLEAN_CHROME_BROWSER_UI_GUIDES_GUIDES_H_
