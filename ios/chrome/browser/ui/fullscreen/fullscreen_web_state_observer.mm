// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/fullscreen/fullscreen_web_state_observer.h"

#include "base/logging.h"
#import "ios/chrome/browser/ui/fullscreen/fullscreen_model.h"
#import "ios/chrome/browser/ui/fullscreen/fullscreen_web_view_proxy_observer.h"
#import "ios/chrome/browser/ui/fullscreen/scoped_fullscreen_disabler.h"
#import "ios/web/public/navigation_item.h"
#import "ios/web/public/navigation_manager.h"
#include "ios/web/public/ssl_status.h"
#import "ios/web/public/web_state/web_state.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {
// Returns whether |web_state|'s visible NavigationItem has a broken SSL.
bool IsWebStateSSLBroken(web::WebState* web_state) {
  if (!web_state)
    return false;
  web::NavigationManager* manager = web_state->GetNavigationManager();
  if (!manager)
    return false;
  web::NavigationItem* item = manager->GetVisibleItem();
  if (!item)
    return false;
  const web::SSLStatus& ssl = item->GetSSL();
  return ssl.security_style == web::SECURITY_STYLE_AUTHENTICATION_BROKEN;
}
}  // namespace

FullscreenWebStateObserver::FullscreenWebStateObserver(
    FullscreenControllerImpl* controller)
    : controller_(controller),
      web_view_proxy_observer_([[FullscreenWebViewProxyObserver alloc]
          initWithController:controller_]) {
  DCHECK(controller_);
}

FullscreenWebStateObserver::~FullscreenWebStateObserver() = default;

void FullscreenWebStateObserver::SetWebState(web::WebState* web_state) {
  if (web_state_ == web_state)
    return;
  if (web_state_)
    web_state_->RemoveObserver(this);
  web_state_ = web_state;
  if (web_state_)
    web_state_->AddObserver(this);
  // Update the model according to the new WebState.
  SetIsLoading(web_state_ ? web_state->IsLoading() : false);
  SetIsSSLBroken(web_state_ ? IsWebStateSSLBroken(web_state_) : false);
  // Update the scroll view replacement handler's proxy.
  web_view_proxy_observer_.proxy =
      web_state_ ? web_state_->GetWebViewProxy() : nil;
}

void FullscreenWebStateObserver::DidFinishNavigation(
    web::WebState* web_state,
    web::NavigationContext* navigation_context) {
  model()->ResetForNavigation();
}

void FullscreenWebStateObserver::DidStartLoading(web::WebState* web_state) {
  SetIsLoading(true);
}

void FullscreenWebStateObserver::DidStopLoading(web::WebState* web_state) {
  SetIsLoading(false);
}

void FullscreenWebStateObserver::DidChangeVisibleSecurityState(
    web::WebState* web_state) {
  SetIsSSLBroken(IsWebStateSSLBroken(web_state));
}

void FullscreenWebStateObserver::WebStateDestroyed(web::WebState* web_state) {
  DCHECK_EQ(web_state, web_state_);
  SetWebState(nullptr);
}

void FullscreenWebStateObserver::SetIsSSLBroken(bool broken) {
  if (!!ssl_disabler_.get() == broken)
    return;
  ssl_disabler_ = broken
                      ? std::make_unique<ScopedFullscreenDisabler>(controller_)
                      : nullptr;
}

void FullscreenWebStateObserver::SetIsLoading(bool loading) {
  if (!!loading_disabler_.get() == loading)
    return;
  loading_disabler_ =
      loading ? std::make_unique<ScopedFullscreenDisabler>(controller_)
              : nullptr;
}
