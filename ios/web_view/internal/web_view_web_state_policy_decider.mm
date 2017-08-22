// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/web_view/internal/web_view_web_state_policy_decider.h"

#import "ios/web_view/public/cwv_navigation_delegate.h"
#import "ios/web_view/public/cwv_web_view.h"

namespace ios_web_view {

WebViewWebStatePolicyDecider::WebViewWebStatePolicyDecider(
    web::WebState* web_state,
    CWVWebView* web_view)
    : web::WebStatePolicyDecider(web_state), web_view_(web_view) {}

bool WebViewWebStatePolicyDecider::ShouldAllowRequest(NSURLRequest* request) {
  id<CWVNavigationDelegate> delegate = web_view_.navigationDelegate;
  if ([delegate
          respondsToSelector:@selector(webView:shouldStartLoadWithRequest:)]) {
    return [delegate webView:web_view_ shouldStartLoadWithRequest:request];
  }
  return true;
}

bool WebViewWebStatePolicyDecider::ShouldAllowResponse(NSURLResponse* response,
                                                       bool for_main_frame) {
  id<CWVNavigationDelegate> delegate = web_view_.navigationDelegate;
  if ([delegate respondsToSelector:@selector
                (webView:shouldContinueLoadWithResponse:forMainFrame:)]) {
    return [delegate webView:web_view_
        shouldContinueLoadWithResponse:response
                          forMainFrame:for_main_frame];
  }
  return true;
}

}  // namespace ios_web_view
