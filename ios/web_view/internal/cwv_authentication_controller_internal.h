// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_WEB_VIEW_INTERNAL_CWV_AUTHENTICATION_CONTROLLER_INTERNAL_H
#define IOS_WEB_VIEW_INTERNAL_CWV_AUTHENTICATION_CONTROLLER_INTERNAL_H

#import "ios/web_view/public/cwv_authentication_controller.h"

namespace ios_web_view {
class WebViewBrowserState;
}  // namespace ios_web_view

@interface CWVAuthenticationController ()

- (instancetype)initWithBrowserState:
    (ios_web_view::WebViewBrowserState*)browserState NS_DESIGNATED_INITIALIZER;

- (void)signInWithIdentity:(CWVIdentity*)identity;
- (void)signOut;

@end

#endif  // IOS_WEB_VIEW_INTERNAL_CWV_AUTHENTICATION_CONTROLLER_INTERNAL_H
