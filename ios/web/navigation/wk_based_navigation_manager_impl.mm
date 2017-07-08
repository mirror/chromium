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

// A simpler wrapper around WKBackForwardList that provides an iterator
// interface over the logical list of backList + currentItem + forwardList.
// It assumes that the underlying WKBackForwardList is not changing for the
// lifespan of this iterator.
class BackForwardListIterator {
 public:
  static bool GO_FORWARD = true;
  static bool GO_BACKWARD = false;

  BackForwardListIterator(const WKBackForwardList* wk_back_forward_list,
                          bool go_forward)
      : list_(wk_back_forward_list),
        go_forward_(go_forward),
        num_traversed_(0),
        current_sub_list_(go_foward ? BACK_LIST : FORWARD_LIST) {}

  const WKBackForwardListItem* next() {
    return go_forward_ ? GetNextForward() : GetNextBackward();
  }

 private:
  enum PositionType { BACK_LIST, CURRENT_ITEM, FORWARD_LIST, END };

  const WKBackForwardListItem* StepForwardInList(id<NSArray> sub_list,
                                                 PositionType next_list) {
    if (num_traversed_ < sub_list.count) {
      return sub_list[num_traversed_++];
    } else {
      current_sub_list_ = next_list;
      return next_list == END ? nil : GetNextForward();
    }
  }

  const WKBackForwardListItem* GetNextForward() {
    const WKBackForwardListItem* result = nil;

    switch (current_sub_list_) {
      case BACK_LIST:
        return StepForwardInList(list_.backList, CURRENT_ITEM);
      case CURRENT_ITEM:
        const WKBackForwardListItem* result = list_.currentItem;
        current_sub_list_ = FORWARD_LIST;
        num_traversed_ = 0;
        return result;
      case FORWARD_LIST:
        return StepForwardInList(list_.forwardList, END);
      default:
        return nil;
    }
  }

  const WKBackForwardListItem* GetNextBackward() {
    switch (current_sub_list_) {
      case FORWARD_LIST:
        if (list_.forwardList.count > num_traversed) {
          return list_.forwardList[list_.forwardList.count - (++num_traversed)];
        } else {
          current_sub_list_ = CURRENT_ITEM;
          return GetNextBackward();
        }
      case CURRENT_ITEM:
        const WKBackForwardListItem* result = list_.currentItem;
        current_sub_list_ = BACK_LIST;
        index_in_sub_list_ = list_.backList.count - 1;
        return result;
      case BACK_LIST:
        if (list_.backList.count > num_traversed_) {
          return list_.backList[list_.backList.count - (++num_traversed)];
        } else {
          current_sub_list_ = END;
          return nil;
        }
      default:
        return nil;
    }
  }

  const WKBackForwardList* list_;
  const bool go_forward_;
  PositionType current_sub_list_;
  size_t num_traversed_;
};

static int GetCurrentItemIndex(id<CRWWebViewNavigationProxy> proxy) {
  if (proxy.backForwardList.currentItem) {
    return static_cast<int>(proxy.backForwardList.backList.count);
  }
  return -1;
}

static bool isEquivalent(const NavigationItemImpl* navigation_item,
                         const WKBackForwardListItem* wk_item) {
  DCHECK(navigation_item);
  DCHECK(wk_item);
  return navigation_item->GetURL() == net::GURLWithNSURL(wk_item.URL);
}

}  // namespace

namespace web {

WKBasedNavigationManagerImpl::WKBasedNavigationManagerImpl()
    : delegate_(nullptr),
      browser_state_(nullptr),
      pending_item_index_(-1),
      previous_item_index_(-1),
      last_committed_item_index_(-1) {
  navigation_items_ = [NSMapTable weakToStrongObjectsMapTable];
}

WKBasedNavigationManagerImpl::~WKBasedNavigationManagerImpl() = default;

void WKBasedNavigationManagerImpl::SetDelegate(
    NavigationManagerDelegate* delegate) {
  delegate_ = delegate;
}

void WKBasedNavigationManagerImpl::SetBrowserState(
    BrowserState* browser_state) {
  browser_state_ = browser_state;
}

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

  // We should no longer need to check if a pending item needs to be created.
  // if (![self shouldCreatePendingItemWithURL:url
  //                               transition:trans
  //                  userAgentOverrideOption:userAgentOverrideOption]) {
  //  return;
  //}

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
}

std::unique_ptr<NavigationItemImpl>
WKBasedNavigationManagerImpl::CreateNavigationItem(
    const GURL& url,
    const Referrer& referrer,
    ui::PageTransition transition,
    NavigationInitiationType initiation_type) const {
  GURL loaded_url(url);

  // NOTE(danyao): add URL rewriting logic
  // BOOL urlWasRewritten = NO;
  // if (_navigationManager) {
  //  std::unique_ptr<std::vector<web::BrowserURLRewriter::URLRewriter>>
  //      transientRewriters = _navigationManager->GetTransientURLRewriters();
  //  if (transientRewriters) {
  //    urlWasRewritten = web::BrowserURLRewriter::RewriteURLWithWriters(
  //        &loaded_url, _browserState, *transientRewriters.get());
  //  }
  //}
  // if (!urlWasRewritten) {
  //  web::BrowserURLRewriter::GetInstance()->RewriteURLIfNecessary(
  //      &loaded_url, _browserState);
  //}

  // if (initiationType == web::NavigationInitiationType::RENDERER_INITIATED &&
  //    loaded_url != url && web::GetWebClient()->IsAppSpecificURL(loaded_url))
  //    {
  //  bool lastCommittedURLIsAppSpecific =
  //      self.lastCommittedItem &&
  //      web::GetWebClient()->IsAppSpecificURL(
  // self.lastCommittedItem->GetURL());
  //  if (!lastCommittedURLIsAppSpecific) {
  //    // The URL should not be changed to app-specific URL if the load was
  //    // renderer-initiated requested by non app-specific URL. Pages with
  //    // app-specific urls have elevated previledges and should not be allowed
  //    // to open app-specific URLs.
  //    loaded_url = url;
  //  }
  //}

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
  last_committed_item_index_ =
      GetCurrentItemIndex(delegate_->GetWebViewNavigationProxy());

  OnNavigationItemCommitted();
}

void WKBasedNavigationManagerImpl::SyncNavigationItemsToWKBackForwardList() {
  // Walk the WKBackForwardList from the end of |forwardList| backwards. Create
  // NavigationItemImpl for each entry that is not in the navigation_items_ map.
  // This method is called under the following scenario:
  // 1. The pending item has just been committed to the new current item: link
  //    the NavigationItemImpl in pending_item_ to |backForwardList.currentItem|
  // 2. Web view only navigations (i.e. iframe navigation, hash change,
  //    pushState, replaceState) have happneed in WKWebView that did not result
  //    in a |startProvisionalNavigation| or |didCommitProvisionalNavigation|
  //    WKNavigationDelegate callback: create new NavigationItemImpl objects.
  //
  // No cleanup is required for this map because NSMapTable automatically takes
  // care of removing the entry when a WKBackForwardListItem is deleted from
  // WKWebView.
  id<CRWWebViewNavigationProxy> proxy = delegate_->GetWebViewNavigationProxy();

  if (pending_item_index_ == -1 && proxy.backForwardList.currentItem &&
      isEquivalent(pending_item_.get(), proxy.backForwardList.currentItem)) {
    NavigationItemImpl* item = new NavigationItemImpl(pending_item_.get());
    [navigation_items_ setObject:item forKey:proxy.backForwardList.currentItem];
  }

  for (size_t count = proxy.backForwardList.forwardList.count; count > 0;
       count--) {
    WKBackForwardListItem* wk_item =
        proxy.backForwardList.forwardList[count - 1];
    const NavigationItemImpl* item = [navigation_items_ objectForKey:wk_item];
    if (item && isEquivalent(item, wk_item)) {
      last_equivalent_item = item;
      break;
    }
  }
}

int WKBasedNavigationManagerImpl::GetIndexForOffset(int offset) const {
  id<CRWWebViewNavigationProxy> proxy = delegate_->GetWebViewNavigationProxy();
  int current_item_index = GetCurrentItemIndex(proxy);
  int result = current_item_index + offset;
  if (result < 0) {
    return INT_MIN;
  } else if (result >= GetItemCount()) {
    return INT_MAX;
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
  DLOG(WARNING) << "Not yet implemented.";
  return nullptr;
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

NavigationItem* WKBasedNavigationManagerImpl::GetItemAtIndex(
    size_t index) const {
  DLOG(WARNING) << "Not yet implemented.";
  return nullptr;
}

int WKBasedNavigationManagerImpl::GetIndexOfItem(
    const NavigationItem* item) const {
  DLOG(WARNING) << "Not yet implemented.";
  return -1;
}

int WKBasedNavigationManagerImpl::GetPendingItemIndex() const {
  return pending_item_index_;
}

int WKBasedNavigationManagerImpl::GetLastCommittedItemIndex() const {
  id<CRWWebViewNavigationProxy> proxy = delegate_->GetWebViewNavigationProxy();
  if (proxy.backForwardList.currentItem) {
    return static_cast<int>(proxy.backForwardList.backList.count);
  }
  return -1;
}

bool WKBasedNavigationManagerImpl::RemoveItemAtIndex(int index) {
  DLOG(WARNING) << "Not yet implemented.";
  return true;
}

bool WKBasedNavigationManagerImpl::CanGoBack() const {
  return [delegate_->GetWebViewNavigationProxy() canGoBack];
}

bool WKBasedNavigationManagerImpl::CanGoForward() const {
  return [delegate_->GetWebViewNavigationProxy() canGoForward];
}

bool WKBasedNavigationManagerImpl::CanGoToOffset(int offset) const {
  int index = GetIndexForOffset(offset);
  return index >= 0 && index < GetItemCount();
}

void WKBasedNavigationManagerImpl::GoBack() {
  [delegate_->GetWebViewNavigationProxy() goBack];
}

void WKBasedNavigationManagerImpl::GoForward() {
  [delegate_->GetWebViewNavigationProxy() goForward];
}

void WKBasedNavigationManagerImpl::GoToIndex(int index) {
  DLOG(WARNING) << "Not yet implemented.";
}

void WKBasedNavigationManagerImpl::Reload(ReloadType reload_type,
                                          bool check_for_reposts) {
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
  DLOG(WARNING) << "Not yet implemented.";
  return -1;
}

}  // namespace web
