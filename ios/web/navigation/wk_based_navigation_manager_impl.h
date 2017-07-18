// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_WEB_NAVIGATION_WK_BASED_NAVIGATION_MANAGER_IMPL_H_
#define IOS_WEB_NAVIGATION_WK_BASED_NAVIGATION_MANAGER_IMPL_H_

#include <stddef.h>

#include <memory>
#include <vector>

#import "base/mac/scoped_nsobject.h"
#include "base/macros.h"
#import "ios/web/navigation/navigation_item_impl.h"
#import "ios/web/navigation/navigation_manager_impl.h"
#include "ios/web/navigation/time_smoother.h"
#include "ios/web/public/reload_type.h"
#include "ui/base/page_transition_types.h"
#include "url/gurl.h"

@class WKBackForwardListItem;

namespace web {
class BrowserState;
class NavigationItem;
struct Referrer;
class SessionStorageBuilder;

// WKBackForwardList based implementation of NavigationManagerImpl.
// This class relies on the following WKWebView APIs, defined by the
// CRWWebViewNavigationProxy protocol:
//   @property backForwardList
//   @property canGoBack
//   @property canGoForward
//   - goBack
//   - goForward
//   - goToBackForwardListItem:
class WKBasedNavigationManagerImpl : public NavigationManagerImpl {
 public:
  WKBasedNavigationManagerImpl();
  ~WKBasedNavigationManagerImpl() override;

  // NavigationManagerImpl:
  void SetSessionController(CRWSessionController* session_controller) override;
  void InitializeSession() override;
  void ReplaceSessionHistory(std::vector<std::unique_ptr<NavigationItem>> items,
                             int current_index) override;
  void OnNavigationItemsPruned(size_t pruned_item_count) override;
  void OnNavigationItemChanged() override;
  void OnNavigationItemCommitted() override;
  CRWSessionController* GetSessionController() const override;
  void AddTransientItem(const GURL& url) override;
  void AddPendingItem(
      const GURL& url,
      const web::Referrer& referrer,
      ui::PageTransition navigation_type,
      NavigationInitiationType initiation_type,
      UserAgentOverrideOption user_agent_override_option) override;
  void CommitPendingItem() override;
  std::unique_ptr<std::vector<BrowserURLRewriter::URLRewriter>>
  GetTransientURLRewriters() override;
  void RemoveTransientURLRewriters() override;
  int GetIndexForOffset(int offset) const override;
  // Returns the previous navigation item in the main frame.
  int GetPreviousItemIndex() const override;

  // NavigationManager:
  BrowserState* GetBrowserState() const override;
  WebState* GetWebState() const override;
  NavigationItem* GetVisibleItem() const override;
  NavigationItem* GetLastCommittedItem() const override;
  // Returns the pending navigation item in the main frame.
  NavigationItem* GetPendingItem() const override;
  NavigationItem* GetTransientItem() const override;
  void DiscardNonCommittedItems() override;
  void LoadURLWithParams(const NavigationManager::WebLoadParams&) override;
  void AddTransientURLRewriter(
      BrowserURLRewriter::URLRewriter rewriter) override;
  int GetItemCount() const override;
  NavigationItem* GetItemAtIndex(size_t index) const override;
  int GetIndexOfItem(const NavigationItem* item) const override;
  int GetPendingItemIndex() const override;
  int GetLastCommittedItemIndex() const override;
  bool RemoveItemAtIndex(int index) override;
  bool CanGoBack() const override;
  bool CanGoForward() const override;
  bool CanGoToOffset(int offset) const override;
  void GoBack() override;
  void GoForward() override;
  void GoToIndex(int index) override;
  NavigationItemList GetBackwardItems() const override;
  NavigationItemList GetForwardItems() const override;
  void CopyStateFromAndPrune(const NavigationManager* source) override;
  bool CanPruneAllButLastCommittedItem() const override;

 private:
  // The SessionStorageBuilder functions require access to private variables of
  // NavigationManagerImpl.
  friend SessionStorageBuilder;

  // NavigationManagerImpl methods used by SessionStorageBuilder.
  NavigationItemImpl* GetNavigationItemImplAtIndex(size_t index) const override;

  // Rebuild NavigationItemImpl objects to sync to WKBackForwardList.
  void SyncNavigationItemsToWKBackForwardList();

  // Returns the absolute index of WKBackForwardList's |currentItem|. Returns -1
  // if |currentItem| is nil.
  int GetWKCurrentItemIndex() const;

  // Returns the WKNavigationItem object at the absolute index. Returns nil if
  // |index| is out of range.
  const WKBackForwardListItem* GetWKItemAtIndex(int index) const;

  NavigationItemImpl* InternalGetItemAtIndex(int index) const;

  // Create a new NavigationItemImpl object using the given parameters.
  std::unique_ptr<NavigationItemImpl> CreateNavigationItem(
      const GURL& url,
      const Referrer& referrer,
      ui::PageTransition transition,
      NavigationInitiationType initiation_type) const;

  // List of transient url rewriters added by |AddTransientURLRewriter()|.
  std::unique_ptr<std::vector<BrowserURLRewriter::URLRewriter>>
      transient_url_rewriters_;

  // The pending main frame navigation item.
  std::unique_ptr<NavigationItemImpl> pending_item_;

  // -1 if pending_item_ represents a new navigation. Otherwise, this is the
  // index of the pending_item in the back-forward list.
  int pending_item_index_;

  // Index of the previous navigation item in the main frame. If there is none,
  // this field will have value -1.
  int previous_item_index_;

  // Index of the last committed item in the main frame. If there is none, this
  // field will equal to -1.
  int last_committed_item_index_;

  // The transient item in main frame.
  std::unique_ptr<NavigationItemImpl> transient_item_;

  // NavigationItemImpl objects associated with the WKBackForwardList items.
  // Note that this list is updated lazily on access. All accessor methods must
  // perform equivalence check before accessing these items.
  std::vector<std::unique_ptr<NavigationItemImpl>> navigation_item_impls_;

  // Time smoother for navigation item timestamps. See comment in
  // navigation_controller_impl.h.
  TimeSmoother time_smoother_;

  DISALLOW_COPY_AND_ASSIGN(WKBasedNavigationManagerImpl);
};

}  // namespace web

#endif  // IOS_WEB_NAVIGATION_WK_BASED_NAVIGATION_MANAGER_IMPL_H_
