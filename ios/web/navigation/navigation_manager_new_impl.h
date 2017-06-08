// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_WEB_NAVIGATION_NAVIGATION_MANAGER_NEW_IMPL_H_
#define IOS_WEB_NAVIGATION_NAVIGATION_MANAGER_NEW_IMPL_H_

#import <WebKit/WebKit.h>
#include <memory>
#include <vector>

#include "base/macros.h"
#import "ios/web/public/navigation_item_list.h"
#import "ios/web/public/navigation_manager.h"
#include "ios/web/public/reload_type.h"

namespace web {
class BrowserState;
class NavigationManagerDelegate;

// A new implementation of NavigationManager on top of WKBackForwardList.
class NavigationManagerNewImpl : public NavigationManager {
 public:
  NavigationManagerNewImpl();
  ~NavigationManagerNewImpl() override;

  // Setters for NavigationManagerDelegate and BrowserState.
  void SetDelegate(NavigationManagerDelegate* delegate);
  void SetBrowserState(BrowserState* browser_state);

  // Initializes a new session history.
  void InitializeSession();

  // Adds a transient item with the given URL. A transient item will be
  // discarded on any navigation.
  void AddTransientItem(const GURL& url);
  // void AddPendingItem(const GURL& url,
  //     const web::Referrer& referrer,
  //     ui::PageTransition navigation_type,
  //     NavigationInitiationType initiation_type,
  //     UserAgentOverrideOption user_agent_override_option);

  // Navigation Manager:
  BrowserState* GetBrowserState() const override;
  WebState* GetWebState() const override;

  NavigationItem* GetVisibleItem() const override;
  NavigationItem* GetLastCommittedItem() const override;
  NavigationItem* GetPendingItem() const override;
  NavigationItem* GetTransientItem() const override;
  int GetItemCount() const override;
  NavigationItem* GetItemAtIndex(size_t index) const override;
  int GetPendingItemIndex() const override;
  int GetLastCommittedItemIndex() const override;
  bool CanGoBack() const override;
  bool CanGoForward() const override;
  bool CanGoToOffset(int offset) const override;
  NavigationItemList GetBackwardItems() const override;
  NavigationItemList GetForwardItems() const override;

  void DiscardNonCommittedItems() override;
  bool RemoveItemAtIndex(int index) override;
  void CopyStateFromAndPrune(const NavigationManager* source) override;
  bool CanPruneAllButLastCommittedItem() const override;

  void LoadURLWithParams(const NavigationManager::WebLoadParams&) override;
  void GoBack() override;
  void GoForward() override;
  void GoToIndex(int index) override;
  void Reload(ReloadType reload_type, bool check_for_reposts) override;

  void AddTransientURLRewriter(
      BrowserURLRewriter::URLRewriter rewriter) override;

  // Called to reset the transient url rewriter list.
  void RemoveTransientURLRewriters();

 private:
  // Helper functions for notifying WebStateObservers of changes.
  // TODO(stuartmorgan): these should become private
  // void onNavigationItemsPruned(size_t pruned_item_count);
  // void onNavigationItemChanged();
  // void onNavigationItemCommitted();

  //// Temporary accessors and content / class pass-throughs.
  //// TODO(stuartmorgan): Re-evaluate this list once the refactorings have
  //// settled down.
  // void LoadURL(const GURL& url,
  //    const Referrer& referrer,
  //    ui::PageTransition type);

  //// TODO(crbug.com/546197): remove once NavigationItem creation occurs in
  ///this / class.
  // std::unique_ptr<std::vector<BrowserURLRewriter::URLRewriter>>
  //  GetTransientURLRewriters();

  //// Returns the navigation index that differs from the current item (or
  ///pending / item if it exists) by the specified |offset|, skipping redirect
  ///navigation / items. The index returned is not guaranteed to be valid. /
  ///TODO(crbug.com/661316): Make this method private once navigation code is /
  ///moved from CRWWebController to NavigationManagerImpl.
  // int GetIndexForOffset(int offset) const;

  // DO_NOT_SUBMIT: this will create new copies of NavigationItem on each call.
  NavigationItem* makeNavigationItem(
      const WKBackForwardListItem* current_item) const;

  // The primary delegate for this manager.
  NavigationManagerDelegate* delegate_;

  // The BrowserState that is associated with this instance.
  BrowserState* browser_state_;

  // List of transient url rewriters added by |AddTransientURLRewriter()|.
  std::unique_ptr<std::vector<BrowserURLRewriter::URLRewriter>>
      transient_url_rewriters_;

  // Implements RAII for NavigationItem.
  // TODO(danyao): this needs to be thread safe.
  mutable std::vector<std::unique_ptr<NavigationItem>> navigation_item_holder_;

  DISALLOW_COPY_AND_ASSIGN(NavigationManagerNewImpl);
};

}  // namespace web

#endif  // IOS_WEB_NAVIGATION_NAVIGATION_MANAGER_NEW_IMPL_H_
