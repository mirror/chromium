// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/web/navigation/wk_based_navigation_manager_impl.h"

#import <Foundation/Foundation.h>
#import <objc/runtime.h>
#include <memory>

#include "base/logging.h"
#include "base/memory/ptr_util.h"
#import "ios/web/navigation/crw_navigation_item_holder.h"
#import "ios/web/navigation/navigation_item_impl.h"
#import "ios/web/navigation/navigation_manager_delegate.h"
#include "ios/web/public/load_committed_details.h"
#import "ios/web/public/navigation_item.h"
#import "ios/web/public/web_client.h"
#import "ios/web/web_state/ui/crw_web_view_navigation_proxy.h"
#import "net/base/mac/url_conversions.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@class CRWSessionController;

// The address of this static char is used to set and get the associated
// NavigationItemImpl object from a WKBackForwardListItem.
static char kNavigationItemKey;

namespace {

void SetNavigationItemInWKItem(
    const WKBackForwardListItem* wk_item,
    std::unique_ptr<web::NavigationItemImpl> nav_item) {
  DCHECK(nav_item->GetURL() == net::GURLWithNSURL(wk_item.URL));
  CRWNavigationItemHolder* holder = [[CRWNavigationItemHolder alloc]
      initWithNavigationItem:std::move(nav_item)];
  objc_setAssociatedObject(wk_item, &kNavigationItemKey, holder,
                           OBJC_ASSOCIATION_RETAIN);
}

web::NavigationItemImpl* GetNavigationItemFromWKItem(
    const WKBackForwardListItem* wk_item) {
  CRWNavigationItemHolder* holder =
      objc_getAssociatedObject(wk_item, &kNavigationItemKey);
  return holder.navigationItem;
}

}  // namespace

namespace web {

WKBasedNavigationManagerImpl::WKBasedNavigationManagerImpl()
    : pending_item_index_(-1),
      previous_item_index_(-1),
      last_committed_item_index_(-1) {}

WKBasedNavigationManagerImpl::~WKBasedNavigationManagerImpl() = default;

void WKBasedNavigationManagerImpl::SetSessionController(
    CRWSessionController* session_controller) {}

void WKBasedNavigationManagerImpl::InitializeSession() {}

void WKBasedNavigationManagerImpl::ReplaceSessionHistory(
    std::vector<std::unique_ptr<NavigationItem>> items,
    int current_index) {
  DLOG(WARNING) << "Not yet implemented.";
}

void WKBasedNavigationManagerImpl::OnNavigationItemsPruned(
    size_t pruned_item_count) {
  delegate_->OnNavigationItemsPruned(pruned_item_count);
}

void WKBasedNavigationManagerImpl::OnNavigationItemChanged() {
  delegate_->OnNavigationItemChanged();
}

void WKBasedNavigationManagerImpl::OnNavigationItemCommitted() {
  LoadCommittedDetails details;
  details.item = GetLastCommittedItem();
  DCHECK(details.item);
  details.previous_item_index = GetPreviousItemIndex();
  if (details.previous_item_index >= 0) {
    NavigationItem* previous_item = GetItemAtIndex(details.previous_item_index);
    DCHECK(previous_item);
    details.previous_url = previous_item->GetURL();
    details.is_in_page = AreUrlsFragmentChangeNavigation(
        details.previous_url, details.item->GetURL());
  } else {
    details.previous_url = GURL();
    details.is_in_page = NO;
  }

  delegate_->OnNavigationItemCommitted(details);
}

CRWSessionController* WKBasedNavigationManagerImpl::GetSessionController()
    const {
  return nil;
}

void WKBasedNavigationManagerImpl::AddTransientItem(const GURL& url) {
  transient_item_ =
      CreateNavigationItem(url, Referrer(), ui::PAGE_TRANSITION_CLIENT_REDIRECT,
                           NavigationInitiationType::USER_INITIATED);
  transient_item_->SetTimestamp(
      time_smoother_.GetSmoothedTime(base::Time::Now()));

  // Transient item is only supposed to be added for pending non-app-specific
  // navigations.
  DCHECK(pending_item_);
  DCHECK(pending_item_->GetUserAgentType() != UserAgentType::NONE);
  transient_item_->SetUserAgentType(pending_item_->GetUserAgentType());
}

void WKBasedNavigationManagerImpl::AddPendingItem(
    const GURL& url,
    const web::Referrer& referrer,
    ui::PageTransition navigation_type,
    NavigationInitiationType initiation_type,
    UserAgentOverrideOption user_agent_override_option) {
  DiscardNonCommittedItems();

  pending_item_ =
      CreateNavigationItem(url, referrer, navigation_type, initiation_type);

  // AddPendingItme is called no later than |didCommitNavigation|. The only time
  // when all three of WKWebView's URL, the pending URL and WKBackForwardList's
  // current item URL are identical before |didCommitNavigation| is when the
  // in-progress navigation is a back-forward navigation. In this case, current
  // item has already been updated to point to the new location in back-forward
  // history, so pending item index should be set to the current item index.
  id<CRWWebViewNavigationProxy> proxy = delegate_->GetWebViewNavigationProxy();
  if (proxy.backForwardList.currentItem &&
      proxy.URL == proxy.backForwardList.currentItem.URL &&
      pending_item_->GetURL() ==
          net::GURLWithNSURL(proxy.backForwardList.currentItem.URL)) {
    pending_item_index_ =
        static_cast<int>(proxy.backForwardList.backList.count);
  } else {
    pending_item_index_ = -1;
  }

  UpdatePendingItemUserAgentType(user_agent_override_option,
                                 // WKBasedNavigationManagerImpl does not track
                                 // native URLs yet so just inherit from the
                                 // last committed item.
                                 // TODO(crbug.com/734150): change this to
                                 // GetLastCommittedNonAppSpecificItem() after
                                 // integrating with native URLs.
                                 GetLastCommittedItem(), pending_item_.get());
}

void WKBasedNavigationManagerImpl::CommitPendingItem() {
  pending_item_->ResetForCommit();
  pending_item_->SetTimestamp(
      time_smoother_.GetSmoothedTime(base::Time::Now()));

  // WKBackForwardList's |currentItem| would have already been updated when this
  // method is called and it is the last committed item.
  id<CRWWebViewNavigationProxy> proxy = delegate_->GetWebViewNavigationProxy();
  SetNavigationItemInWKItem(proxy.backForwardList.currentItem,
                            std::move(pending_item_));

  pending_item_index_ = -1;
  previous_item_index_ = last_committed_item_index_;
  last_committed_item_index_ = GetWKCurrentItemIndex();

  OnNavigationItemCommitted();
}

int WKBasedNavigationManagerImpl::GetIndexForOffset(int offset) const {
  int result = (pending_item_index_ == -1) ? GetLastCommittedItemIndex()
                                           : pending_item_index_;

  if (offset < 0 && GetTransientItem() && pending_item_index_ == -1) {
    // Going back from transient item that added to the end navigation stack
    // is a matter of discarding it as there is no need to move navigation
    // index back.
    offset++;
  }
  result += offset;
  if (result > GetItemCount() /* overflow */) {
    result = INT_MIN;
  }
  return result;
}

int WKBasedNavigationManagerImpl::GetPreviousItemIndex() const {
  return previous_item_index_;
}

BrowserState* WKBasedNavigationManagerImpl::GetBrowserState() const {
  return browser_state_;
}

WebState* WKBasedNavigationManagerImpl::GetWebState() const {
  return delegate_->GetWebState();
}

NavigationItem* WKBasedNavigationManagerImpl::GetVisibleItem() const {
  DLOG(WARNING) << "Not yet implemented.";
  return nullptr;
}

NavigationItem* WKBasedNavigationManagerImpl::GetLastCommittedItem() const {
  int index = GetLastCommittedItemIndex();
  return index == -1 ? nullptr : GetItemAtIndex(static_cast<size_t>(index));
}

NavigationItem* WKBasedNavigationManagerImpl::GetPendingItem() const {
  return pending_item_.get();
}

NavigationItem* WKBasedNavigationManagerImpl::GetTransientItem() const {
  return transient_item_.get();
}

void WKBasedNavigationManagerImpl::DiscardNonCommittedItems() {
  pending_item_.reset();
  transient_item_.reset();
}

void WKBasedNavigationManagerImpl::LoadURLWithParams(
    const NavigationManager::WebLoadParams&) {
  DLOG(WARNING) << "Not yet implemented.";
}


int WKBasedNavigationManagerImpl::GetItemCount() const {
  id<CRWWebViewNavigationProxy> proxy = delegate_->GetWebViewNavigationProxy();
  if (proxy) {
    int count_current_page = proxy.backForwardList.currentItem ? 1 : 0;
    return static_cast<int>(proxy.backForwardList.backList.count) +
           count_current_page +
           static_cast<int>(proxy.backForwardList.forwardList.count);
  }

  // If WebView has not been created, it's fair to say navigation has 0 item.
  return 0;
}

NavigationItem* WKBasedNavigationManagerImpl::GetItemAtIndex(
    size_t index) const {
  const WKBackForwardListItem* wk_item = GetWKItemAtIndex(index);
  NavigationItemImpl* item = GetNavigationItemFromWKItem(wk_item);

  if (item) {
    return item;
  }

  // TODO(crbug.com/734150): Add a stat counter to track rebuilding frequency.
  const_cast<WKBasedNavigationManagerImpl*>(this)
      ->SyncNavigationItemsToWKBackForwardList();
  return GetNavigationItemFromWKItem(wk_item);
}

int WKBasedNavigationManagerImpl::GetIndexOfItem(
    const NavigationItem* item) const {
  DLOG(WARNING) << "Not yet implemented.";
  return -1;
}

int WKBasedNavigationManagerImpl::GetPendingItemIndex() const {
  if (GetPendingItem()) {
    if (pending_item_index_ != -1) {
      return pending_item_index_;
    }
    // TODO(crbug.com/665189): understand why last committed item index is
    // returned here.
    return GetLastCommittedItemIndex();
  }
  return -1;
}

int WKBasedNavigationManagerImpl::GetLastCommittedItemIndex() const {
  return last_committed_item_index_;
}

bool WKBasedNavigationManagerImpl::RemoveItemAtIndex(int index) {
  DLOG(WARNING) << "Not yet implemented.";
  return true;
}

bool WKBasedNavigationManagerImpl::CanGoBack() const {
  return CanGoToOffset(-1);
}

bool WKBasedNavigationManagerImpl::CanGoForward() const {
  return CanGoToOffset(1);
}

bool WKBasedNavigationManagerImpl::CanGoToOffset(int offset) const {
  int index = GetIndexForOffset(offset);
  return index >= 0 && index < GetItemCount();
}

void WKBasedNavigationManagerImpl::GoBack() {
  if (transient_item_) {
    transient_item_.reset();
    return;
  }
  [delegate_->GetWebViewNavigationProxy() goBack];
}

void WKBasedNavigationManagerImpl::GoForward() {
  [delegate_->GetWebViewNavigationProxy() goForward];
}

void WKBasedNavigationManagerImpl::GoToIndex(int index) {
  DLOG(WARNING) << "Not yet implemented.";
}

NavigationItemList WKBasedNavigationManagerImpl::GetBackwardItems() const {
  DLOG(WARNING) << "Not yet implemented.";
  return NavigationItemList();
}

NavigationItemList WKBasedNavigationManagerImpl::GetForwardItems() const {
  DLOG(WARNING) << "Not yet implemented.";
  return NavigationItemList();
}

void WKBasedNavigationManagerImpl::CopyStateFromAndPrune(
    const NavigationManager* source) {
  DLOG(WARNING) << "Not yet implemented.";
}

bool WKBasedNavigationManagerImpl::CanPruneAllButLastCommittedItem() const {
  DLOG(WARNING) << "Not yet implemented.";
  return true;
}

NavigationItemImpl* WKBasedNavigationManagerImpl::GetNavigationItemImplAtIndex(
    size_t index) const {
  DLOG(WARNING) << "Not yet implemented.";
  return nullptr;
}

void WKBasedNavigationManagerImpl::SyncNavigationItemsToWKBackForwardList() {
  const WKBackForwardListItem* prev_wk_item = nil;
  for (int index = 0; index < GetItemCount(); index++) {
    const WKBackForwardListItem* wk_item = GetWKItemAtIndex(index);
    const NavigationItemImpl* nav_item = GetNavigationItemFromWKItem(wk_item);
    if (!nav_item) {
      std::unique_ptr<NavigationItemImpl> item = CreateNavigationItem(
          net::GURLWithNSURL(wk_item.URL),
          (prev_wk_item ? web::Referrer(net::GURLWithNSURL(prev_wk_item.URL),
                                        web::ReferrerPolicyAlways)
                        : web::Referrer()),
          ui::PageTransition::PAGE_TRANSITION_LINK,
          NavigationInitiationType::RENDERER_INITIATED);
      SetNavigationItemInWKItem(wk_item, std::move(item));
    }

    prev_wk_item = wk_item;
  }

  // Infer last committed item index if it is not set.
  if (last_committed_item_index_ == -1) {
    last_committed_item_index_ = GetWKCurrentItemIndex();
  }
}

int WKBasedNavigationManagerImpl::GetWKCurrentItemIndex() const {
  id<CRWWebViewNavigationProxy> proxy = delegate_->GetWebViewNavigationProxy();
  if (proxy.backForwardList.currentItem) {
    return static_cast<int>(proxy.backForwardList.backList.count);
  }
  return -1;
}

const WKBackForwardListItem* WKBasedNavigationManagerImpl::GetWKItemAtIndex(
    int index) const {
  if (index < 0 || index >= GetItemCount()) {
    return nil;
  }

  // Convert the index to an offset relative to backForwardList.currentItem (
  // which is also the last committed item), then use WKBackForwardList API to
  // retrieve the item.
  int offset = index - GetWKCurrentItemIndex();
  id<CRWWebViewNavigationProxy> proxy = delegate_->GetWebViewNavigationProxy();
  return [proxy.backForwardList itemAtIndex:offset];
}

}  // namespace web
