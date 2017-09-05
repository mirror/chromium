// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_WEB_VIEW_PUBLIC_CWV_WEB_VIEW_SYNC_H_
#define IOS_WEB_VIEW_PUBLIC_CWV_WEB_VIEW_SYNC_H_

#import "ios/web_view/public/cwv_web_view.h"

@class CWVAuthenticationController;

@interface CWVWebView (Sync)

// This web view's authentication controller.
@property(nonatomic, readonly)
    CWVAuthenticationController* authenticationController;

@end

#endif  // IOS_WEB_VIEW_PUBLIC_CWV_WEB_VIEW_SYNC_H_
