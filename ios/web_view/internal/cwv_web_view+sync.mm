// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/web_view/public/cwv_web_view+sync.h"

#import <objc/runtime.h>

#import "ios/web_view/internal/cwv_web_view_configuration_internal.h"
#import "ios/web_view/internal/signin/cwv_authentication_controller_internal.h"

@implementation CWVWebView (Sync)

- (CWVAuthenticationController*)authenticationController {
  CWVAuthenticationController* authenticationController =
      objc_getAssociatedObject(self, @selector(authenticationController));
  if (!authenticationController) {
    ios_web_view::WebViewBrowserState* browserState =
        self.configuration.browserState;
    authenticationController =
        [[CWVAuthenticationController alloc] initWithBrowserState:browserState];
    objc_setAssociatedObject(self, @selector(authenticationController),
                             authenticationController,
                             OBJC_ASSOCIATION_RETAIN_NONATOMIC);
  }
  return authenticationController;
}

@end
