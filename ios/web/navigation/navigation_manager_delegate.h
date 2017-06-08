// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_WEB_NAVIGATION_NAVIGATION_MANAGER_DELEGATE_H_
#define IOS_WEB_NAVIGATION_NAVIGATION_MANAGER_DELEGATE_H_

#import <WebKit/WebKit.h>
#include <stddef.h>

#import "ios/web/public/navigation_manager.h"
#import "ios/web/web_state/ui/crw_web_controller.h"

namespace web {

struct LoadCommittedDetails;
class WebState;

// Delegate for NavigationManager to hand off parts of the navigation flow.
class NavigationManagerDelegate {
 public:
  virtual ~NavigationManagerDelegate() {}

  // Instructs the delegate to begin navigating to the item with index.
  // TODO(crbug.com/661316): Remove this method once all navigation code is
  // moved to NavigationManagerImpl.
  virtual void GoToIndex(int index) = 0;

  // Instructs the delegate to load the URL.
  virtual void LoadURLWithParams(const NavigationManager::WebLoadParams&) = 0;

  // Instructs the delegate to reload.
  virtual void Reload() = 0;

  // Informs the delegate that committed navigation items have been pruned.
  virtual void OnNavigationItemsPruned(size_t pruned_item_count) = 0;

  // Informs the delegate that a navigation item has been changed.
  virtual void OnNavigationItemChanged() = 0;

  // Informs the delegate that a navigation item has been commited.
  virtual void OnNavigationItemCommitted(
      const LoadCommittedDetails& load_details) = 0;

  // Returns the WebState associated with this delegate.
  virtual WebState* GetWebState() = 0;

  // NOTE(danyao): temporary workaround to expose CRWWebController.
  virtual CRWWebController* GetWebController() = 0;

  // NOTE(danyao): temporary workaround to expose WKWebView.
  virtual WKWebView* GetWKWebView() = 0;
};

}  // namespace web

#endif  // IOS_WEB_NAVIGATION_NAVIGATION_MANAGER_DELEGATE_H_
