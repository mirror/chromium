// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/passwords/credential_manager.h"

#include "base/mac/foundation_util.h"
#import "ios/chrome/browser/passwords/js_credential_manager.h"
#import "ios/web/public/url_scheme_util.h"
#import "ios/web/public/web_state/js/crw_js_injection_receiver.h"
#include "ios/web/public/web_state/web_state.h"
#import "ios/web/public/web_state/web_state_observer_bridge.h"
#include "url/gurl.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@interface CredentialManager ()<CRWWebStateObserver> {
}

@property(nonatomic, weak) JSCredentialManager* jsManager;

@end

@implementation CredentialManager

@synthesize webState = _webState;
@synthesize browserState = _browserState;
@synthesize jsManager = _jsManager;

- (instancetype)initWithBrowserState:(ios::ChromeBrowserState*)browserState
                            webState:(web::WebState*)webState {
  if ((self = [super init])) {
    _browserState = browserState;
    _webState = webState;
  }

  return self;
}

- (instancetype)init {
  NOTREACHED();
  return nil;
}

@end
