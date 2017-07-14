// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/passwords/credential_manager.h"

#import "base/mac/bind_objc_block.h"
#include "base/mac/foundation_util.h"
#import "ios/chrome/browser/passwords/js_credential_manager.h"
#import "ios/web/public/url_scheme_util.h"
#import "ios/web/public/web_state/js/crw_js_injection_receiver.h"
#import "ios/web/public/web_state/ui/crw_web_view_proxy.h"
#include "ios/web/public/web_state/url_verification_constants.h"
#include "ios/web/public/web_state/web_state.h"
#import "ios/web/public/web_state/web_state_observer_bridge.h"

@interface CredentialManager ()<CRWWebStateObserver> {
  web::WebState* _webState;

  std::unique_ptr<web::WebStateObserverBridge> _webStateObserver;

  // Boolean to track if the current WebState is enabled (JS callback is set
  // up).
  BOOL _webStateEnabled;
}

@property(nonatomic, weak) JSCredentialManager* jsManager;

@end

@implementation CredentialManager

@synthesize webState = _webState;
@synthesize browserState = _browserState;
@synthesize jsManager = _jsManager;

- (instancetype)initWithBrowserState:(ios::ChromeBrowserState*)browserState {
  if ((self = [super init])) {
    _browserState = browserState;
  }

  return self;
}

- (instancetype)init {
  NOTREACHED();
  return nil;
}

- (void)setWebState:(web::WebState*)webState {
  [self disconnectWebState];
  if (webState) {
    _jsManager = base::mac::ObjCCastStrict<JSCredentialManager>(
        [webState->GetJSInjectionReceiver()
            instanceOfClass:[JSCredentialManager class]]);
    _webState = webState;
    // _webStateObserver will start invoking the CRWWebStateObserver methods
    // implemented by this class.
    _webStateObserver.reset(new web::WebStateObserverBridge(webState, self));
    [self enableCurrentWebState];
  } else {
    _webState = nullptr;
  }
}

- (void)enableCurrentWebState {
  if (![self webState]) {
    return;
  }

  if (!_webStateEnabled) {
    _webStateEnabled = YES;
  }
}

- (void)disableCurrentWebState {
  if (_webState && _webStateEnabled) {
    _webStateEnabled = NO;
  }
}

- (void)disconnectWebState {
  if (_webState) {
    _jsManager = nil;
    _webStateObserver.reset();
    [self disableCurrentWebState];
  }
}

- (void)webState:(web::WebState*)webState didLoadPageWithSuccess:(BOOL)success {
  const GURL& url = _webState->GetVisibleURL();
  // This prevents from trying to inject JavaScript onto New Tab,
  // Preferences etc.
  if (!web::UrlHasWebScheme(url) || !_webState->ContentIsHTML()) {
    return;
  }
  [_jsManager inject];
}

#pragma mark - CRWWebStateObserver methods

- (void)webState:(web::WebState*)webState
    didCommitNavigationWithDetails:
        (const web::LoadCommittedDetails&)load_details {
  [self enableCurrentWebState];
}

@end
