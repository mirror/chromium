// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_WEB_VIEW_INTERNAL_WEB_VIEW_SYNC_INITIALZER_H_
#define IOS_WEB_VIEW_INTERNAL_WEB_VIEW_SYNC_INITIALZER_H_

class PrefRegistrySimple;
namespace user_prefs {
class PrefRegistrySyncable;
}

namespace ios_web_view {

class WebViewBrowserState;

// Used to initialize objects related to signin and sync.
class WebViewSyncInitializer {
 public:
  // Performs initialization needed before main().
  static void InitializePreMain();

  // Starts any services that need to be initialized with the browser state.
  static void InitializeServicesForBrowserState(
      WebViewBrowserState* browser_state);

  // Registers local preferences.
  static void RegisterLocalPrefs(PrefRegistrySimple* registry);

  // Registers syncable preferences.
  static void RegisterBrowserPrefs(user_prefs::PrefRegistrySyncable* registry);
};

}  // namespace ios_web_view

#endif  // IOS_WEB_VIEW_INTERNAL_WEB_VIEW_SYNC_INITIALZER_H_
