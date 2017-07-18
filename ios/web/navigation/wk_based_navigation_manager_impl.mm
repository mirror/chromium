// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/web/navigation/wk_based_navigation_manager_impl.h"

#import <Foundation/Foundation.h>
#include <memory>

#include "base/logging.h"
#include "base/memory/ptr_util.h"
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

namespace {

static bool isEquivalent(const web::NavigationItemImpl* navigation_item,
                         const WKBackForwardListItem* wk_item) {
  return navigation_item && wk_item &&
         navigation_item->GetURL() == net::GURLWithNSURL(wk_item.URL);
}

}  // namespace

namespace web {

WKBasedNavigationManagerImpl::WKBasedNavigationManagerImpl()
    : NavigationManagerImpl(),
      pending_item_index_(-1),
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

  // AddPendingItem is always called in |startProvisionalNavigation|. Under
  // back-forward navigation, WKBackForwardList's current item pointer is
  // updated before the |startProvisionalNavigation| callback, so we can infer
  // that the current navigation is a back-forward navigation if the pending
  // item URL is identical to the current item URL.
  // NOTE(danyao): Think a bit more about how URL rewriting can throw this off.
  id<CRWWebViewNavigationProxy> proxy = delegate_->GetWebViewNavigationProxy();
  if (proxy.backForwardList.currentItem &&
      isEquivalent(pending_item_.get(), proxy.backForwardList.currentItem)) {
    pending_item_index_ =
        static_cast<int>(proxy.backForwardList.backList.count);
  } else {
    pending_item_index_ = -1;
  }

  // |user_agent_override_option| must be INHERIT if |pending_item|'s
  // UserAgentType is NONE, as requesting a desktop or mobile user agent should
  // be disabled for app-specific URLs.
  DCHECK(pending_item_->GetUserAgentType() != UserAgentType::NONE ||
         user_agent_override_option == UserAgentOverrideOption::INHERIT);

  // Newly created pending items are created with UserAgentType::NONE for native
  // pages or UserAgentType::MOBILE for non-native pages.  If the pending item's
  // URL is non-native, check which user agent type it should be created with
  // based on |user_agent_override_option|.
  DCHECK_NE(UserAgentType::DESKTOP, pending_item_->GetUserAgentType());
  if (pending_item_->GetUserAgentType() == UserAgentType::NONE)
    return;

  switch (user_agent_override_option) {
    case UserAgentOverrideOption::DESKTOP:
      pending_item_->SetUserAgentType(UserAgentType::DESKTOP);
      break;
    case UserAgentOverrideOption::MOBILE:
      pending_item_->SetUserAgentType(UserAgentType::MOBILE);
      break;
    case UserAgentOverrideOption::INHERIT: {
      // WKBasedNavigationManagerImpl does not track native URLs yet so just
      // inherit from the last committed item.
      // TODO(crbug.com/734150): fix this after integrating with native views.
      NavigationItem* last_committed_item = GetLastCommittedItem();
      DCHECK(!last_committed_item ||
             last_committed_item->GetUserAgentType() != UserAgentType::NONE);
      if (last_committed_item) {
        pending_item_->SetUserAgentType(
            last_committed_item->GetUserAgentType());
      }
      break;
    }
  }
}

std::unique_ptr<NavigationItemImpl>
WKBasedNavigationManagerImpl::CreateNavigationItem(
    const GURL& url,
    const Referrer& referrer,
    ui::PageTransition transition,
    NavigationInitiationType initiation_type) const {
  GURL loaded_url(url);

  bool url_was_rewritten = false;
  if (transient_url_rewriters_ && (*transient_url_rewriters_).size()) {
    url_was_rewritten = web::BrowserURLRewriter::RewriteURLWithWriters(
        &loaded_url, browser_state_, *transient_url_rewriters_);
  }

  if (!url_was_rewritten) {
    web::BrowserURLRewriter::GetInstance()->RewriteURLIfNecessary(
        &loaded_url, browser_state_);
  }

  if (initiation_type == web::NavigationInitiationType::RENDERER_INITIATED &&
      loaded_url != url && web::GetWebClient()->IsAppSpecificURL(loaded_url)) {
    const NavigationItem* last_committed_item = GetLastCommittedItem();
    if (!last_committed_item ||
        !web::GetWebClient()->IsAppSpecificURL(last_committed_item->GetURL())) {
      // The URL should not be changed to app-specific URL if the load was
      // renderer-initiated requested by non app-specific URL. Pages with
      // app-specific URLs have elevated previledges and should not be allowed
      // to open app-specific URLs.
      loaded_url = url;
    }
  }

  auto item = base::MakeUnique<NavigationItemImpl>();
  item->SetOriginalRequestURL(loaded_url);
  item->SetURL(loaded_url);
  item->SetReferrer(referrer);
  item->SetTransitionType(transition);
  item->SetNavigationInitiationType(initiation_type);
  if (web::GetWebClient()->IsAppSpecificURL(loaded_url)) {
    item->SetUserAgentType(UserAgentType::NONE);
  }
  return item;
}

void WKBasedNavigationManagerImpl::CommitPendingItem() {
  pending_item_->ResetForCommit();
  pending_item_->SetTimestamp(
      time_smoother_.GetSmoothedTime(base::Time::Now()));
  SyncNavigationItemsToWKBackForwardList();

  pending_item_.reset();
  pending_item_index_ = -1;
  previous_item_index_ = last_committed_item_index_;
  // WKBackForwardList's |currentItem| would have already been updated when this
  // method is called and it is the last committed item.
  last_committed_item_index_ = GetWKCurrentItemIndex();

  OnNavigationItemCommitted();
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

void WKBasedNavigationManagerImpl::SyncNavigationItemsToWKBackForwardList() {
  // With main frame navigations, WKBackForwardList and |navigation_item_impls_|
  // should stay in sync. Navigations in iframes and History API calls can cause
  // the two to get out of sync at the tail. We use the following heuristics to
  // resync |navigation_item_impls_| to WKBackForwardList, which is considered
  // the ground truth:
  // 1. Web view only navigations (i.e. iframe navigation, hash change,
  //    pushState, replaceState) occurred in WKWebView that did not result
  //    in a |startProvisionalNavigation| or |didCommitProvisionalNavigation|
  //    WKNavigationDelegate callback: create new NavigationItemImpl objects,
  //    inferring the referrer property.
  // 2. Pending item for a new main frame navigation has just been promoted to
  //    |currentItem| in WKWebView: insert pending item at the current item
  //    index. There may be web-view only items in WKBackForwardList before the
  //    current item that need syncing as well. We can get into this state after
  //    the following events: main frame nav -> iframe nav -> main frame nav.
  int item_count = GetItemCount();
  navigation_item_impls_.resize(item_count);

  // Infer last committed index if it is not set.
  if (last_committed_item_index_ == -1) {
    last_committed_item_index_ = GetWKCurrentItemIndex();
  }

  int last_equivalent_index = -1;
  for (int index = 0; index < item_count; index++) {
    const WKBackForwardListItem* wk_item = GetWKItemAtIndex(index);
    const NavigationItemImpl* item = InternalGetItemAtIndex(index);
    if (!isEquivalent(item, wk_item)) {
      break;
    }
    last_equivalent_index = index;
  }

  if (last_equivalent_index == item_count - 1) {
    return;
  }

  // Everything after |last_equivalent_index| needs to be rebuilt.
  const WKBackForwardListItem* prev_wk_item =
      GetWKItemAtIndex(last_equivalent_index);
  for (int index = last_equivalent_index + 1; index < GetItemCount(); index++) {
    const WKBackForwardListItem* wk_item = GetWKItemAtIndex(index);

    // If the existing pending item is for a new navigation (i.e.
    // pending_item_index_ == -1) and it appears to have been committed to
    // |currentItem|, then instead of creating a new NavigationItem, use pending
    // item because it has more information.
    if (index == GetWKCurrentItemIndex() && pending_item_index_ == -1 &&
        isEquivalent(pending_item_.get(), wk_item)) {
      navigation_item_impls_[index] =
          base::MakeUnique<NavigationItemImpl>(*pending_item_);
    } else {
      std::unique_ptr<NavigationItemImpl> item = CreateNavigationItem(
          net::GURLWithNSURL(wk_item.URL),
          (prev_wk_item ? web::Referrer(net::GURLWithNSURL(prev_wk_item.URL),
                                        web::ReferrerPolicyAlways)
                        : web::Referrer()),
          ui::PageTransition::PAGE_TRANSITION_LINK,
          NavigationInitiationType::RENDERER_INITIATED);
      navigation_item_impls_[index] = std::move(item);
    }
    prev_wk_item = wk_item;
  }
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

void WKBasedNavigationManagerImpl::AddTransientURLRewriter(
    BrowserURLRewriter::URLRewriter rewriter) {
  DCHECK(rewriter);
  if (!transient_url_rewriters_) {
    transient_url_rewriters_.reset(
        new std::vector<BrowserURLRewriter::URLRewriter>());
  }
  transient_url_rewriters_->push_back(rewriter);
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

NavigationItemImpl* WKBasedNavigationManagerImpl::InternalGetItemAtIndex(
    int index) const {
  if (index < 0 || index >= static_cast<int>(navigation_item_impls_.size())) {
    return nullptr;
  }

  NavigationItemImpl* item = navigation_item_impls_[index].get();
  return isEquivalent(item, GetWKItemAtIndex(index)) ? item : nullptr;
}

NavigationItem* WKBasedNavigationManagerImpl::GetItemAtIndex(
    size_t index) const {
  NavigationItemImpl* item = InternalGetItemAtIndex(index);

  if (item) {
    return item;
  } else {
    // TODO(crbug.com/734150): Add a stat counter to track rebuilding frequency.
    const_cast<WKBasedNavigationManagerImpl*>(this)
        ->SyncNavigationItemsToWKBackForwardList();
    return InternalGetItemAtIndex(index);
  }
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

int WKBasedNavigationManagerImpl::GetWKCurrentItemIndex() const {
  id<CRWWebViewNavigationProxy> proxy = delegate_->GetWebViewNavigationProxy();
  if (proxy.backForwardList.currentItem) {
    return static_cast<int>(proxy.backForwardList.backList.count);
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

void WKBasedNavigationManagerImpl::RemoveTransientURLRewriters() {
  transient_url_rewriters_.reset();
}

std::unique_ptr<std::vector<BrowserURLRewriter::URLRewriter>>
WKBasedNavigationManagerImpl::GetTransientURLRewriters() {
  return std::move(transient_url_rewriters_);
};

NavigationItemImpl* WKBasedNavigationManagerImpl::GetNavigationItemImplAtIndex(
    size_t index) const {
  DLOG(WARNING) << "Not yet implemented.";
  return nullptr;
}

int WKBasedNavigationManagerImpl::GetPreviousItemIndex() const {
  return previous_item_index_;
}

}  // namespace web
