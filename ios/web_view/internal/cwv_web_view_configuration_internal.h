// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_WEB_VIEW_INTERNAL_CWV_WEB_VIEW_CONFIGURATION_INTERNAL_H_
#define IOS_WEB_VIEW_INTERNAL_CWV_WEB_VIEW_CONFIGURATION_INTERNAL_H_

#import "ios/web_view/public/cwv_web_view_configuration.h"

namespace ios_web_view {
class WebViewBrowserState;
}  // namespace ios_web_view

@class CWVWebView;

@interface CWVWebViewConfiguration ()

// Calls |shutDown|.
+ (void)shutDown;

// The browser state associated with this configuration.
@property(nonatomic, readonly, nonnull)
    ios_web_view::WebViewBrowserState* browserState;

- (void)addWebView:(nonnull CWVWebView*)webView;

// Destroys |browserState|.
- (void)shutDown;

@end

#endif  // IOS_WEB_VIEW_INTERNAL_CWV_WEB_VIEW_CONFIGURATION_INTERNAL_H_
