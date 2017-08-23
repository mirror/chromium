// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_WEB_VIEW_INTERNAL_WEB_VIEW_SYNC_INITIALZER_H_
#define IOS_WEB_VIEW_INTERNAL_WEB_VIEW_SYNC_INITIALZER_H_

class PrefRegistrySimple;

namespace ios_web_view {

class WebViewBrowserState;

class WebViewSyncInitializer {
public:
  static void InitializeFactories();
  static void InitializeServicesForBrowserState(WebViewBrowserState* browser_state);
  static void RegisterPrefs(PrefRegistrySimple* registry);
};

}  // namespace ios_web_view

#endif  // IOS_WEB_VIEW_INTERNAL_WEB_VIEW_SYNC_INITIALZER_H_
