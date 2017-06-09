// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/web/navigation/navigation_manager_new_impl.h"

#include <stddef.h>
#include <utility>

#include "base/logging.h"
#include "base/mac/bundle_locations.h"
#include "base/strings/sys_string_conversions.h"
#import "ios/web/navigation/navigation_manager_delegate.h"
#import "ios/web/public/navigation_item.h"
#import "ios/web/public/navigation_item_list.h"
#import "ios/web/public/navigation_manager.h"
#include "ios/web/public/referrer_util.h"
#include "ios/web/public/reload_type.h"
#import "ios/web/public/web_state/web_state.h"
#import "ios/web/web_state/ui/crw_web_controller.h"  // Bad bad bad!!!
#include "net/base/mac/url_conversions.h"

namespace {
NSString* const kReferrerHeaderName = @"Referer";  // [sic]
}  // anonymous namespace

namespace web {

NavigationManagerNewImpl::NavigationManagerNewImpl()
    : delegate_(nullptr), browser_state_(nullptr) {}

NavigationManagerNewImpl::~NavigationManagerNewImpl() {}

void NavigationManagerNewImpl::SetDelegate(
    NavigationManagerDelegate* delegate) {
  delegate_ = delegate;
}

void NavigationManagerNewImpl::SetBrowserState(BrowserState* browser_state) {
  browser_state_ = browser_state;
}

void NavigationManagerNewImpl::InitializeSession() {
  // Legacy method to initialize CRWSessionController. Not needed anymore.
}

void NavigationManagerNewImpl::AddTransientItem(const GURL& url) {
  DCHECK(false) << "Not implemented.";
}

BrowserState* NavigationManagerNewImpl::GetBrowserState() const {
  return browser_state_;
}

WebState* NavigationManagerNewImpl::GetWebState() const {
  return delegate_->GetWebState();
}

NavigationItem* NavigationManagerNewImpl::GetVisibleItem() const {
  // TODO(danyao): probably need some kind of map in this class that maps
  // WKBackForwardListItem addresses to NavigationItem objects instead of
  // creating a new one every time.
  const WKWebView* web_view = delegate_->GetWKWebView();
  if (web_view) {
    return makeNavigationItem([[web_view backForwardList] currentItem]);
  }
  return nullptr;
}

NavigationItem* NavigationManagerNewImpl::GetLastCommittedItem() const {
  DLOG(WARNING) << "DW -- GetLastCommittedItem called from "
                << base::SysNSStringToUTF8([[NSThread callStackSymbols]
                       componentsJoinedByString:@"\n"]);

  // TODO(danyao): this is probably not quite right. Check when visible item
  // may be the pending item.
  return GetVisibleItem();
}

NavigationItem* NavigationManagerNewImpl::GetPendingItem() const {
  DCHECK(false) << "Not implemented.";
  return nullptr;
}
NavigationItem* NavigationManagerNewImpl::GetTransientItem() const {
  DCHECK(false) << "Not implemented.";
  return nullptr;
}

int NavigationManagerNewImpl::GetItemCount() const {
  const WKWebView* web_view = delegate_->GetWKWebView();

  if (web_view) {
    const WKBackForwardList* back_forward_list = web_view.backForwardList;
    NSUInteger count_current_page = back_forward_list.currentItem ? 1 : 0;
    return (int)(back_forward_list.backList.count + count_current_page +
                 back_forward_list.forwardList.count);
  } else {
    // If WebView has not been created, it's fair to say navigation has 0 item.
    return 0;
  }
}

// index is treated as an absolute index.
NavigationItem* NavigationManagerNewImpl::GetItemAtIndex(size_t index) const {
  const WKWebView* web_view = delegate_->GetWKWebView();
  if (!web_view) {
    return nullptr;
  }

  if (index > (size_t)GetItemCount()) {
    return nullptr;
  }

  const WKBackForwardList* back_forward_list = web_view.backForwardList;
  const NSArray<WKBackForwardListItem*>* back_list = back_forward_list.backList;
  const NSArray<WKBackForwardListItem*>* forward_list =
      back_forward_list.forwardList;

  // TODO(danyao): This is SUPER bad! If we traverse the list multiple times
  // we'll be building up more and more memory holding NavigationItems.
  if (index < back_list.count) {
    return makeNavigationItem(back_list[index]);
  } else if (index == back_list.count) {
    CHECK(back_forward_list.currentItem);
    return makeNavigationItem(back_forward_list.currentItem);
  } else {
    return makeNavigationItem(forward_list[index - back_list.count - 1]);
  }
}

int NavigationManagerNewImpl::GetPendingItemIndex() const {
  DCHECK(false) << "Not implemented.";
  return -1;
}

int NavigationManagerNewImpl::GetLastCommittedItemIndex() const {
  const WKWebView* web_view = delegate_->GetWKWebView();
  if (web_view) {
    // TODO(danyao): double check that current item is the first item on the
    // forwardList.
    return (int)[[[web_view backForwardList] backList] count];
  }
  return -1;
}

bool NavigationManagerNewImpl::CanGoBack() const {
  return [delegate_->GetWKWebView() canGoBack];
}

bool NavigationManagerNewImpl::CanGoForward() const {
  return [delegate_->GetWKWebView() canGoForward];
}

bool NavigationManagerNewImpl::CanGoToOffset(int offset) const {
  DCHECK(false) << "Not implemented.";
  return false;
}

NavigationItemList NavigationManagerNewImpl::GetBackwardItems() const {
  DCHECK(false) << "Not implemented.";
  return NavigationItemList();
}

NavigationItemList NavigationManagerNewImpl::GetForwardItems() const {
  DCHECK(false) << "Not implemented.";
  return NavigationItemList();
}

void NavigationManagerNewImpl::DiscardNonCommittedItems() {
  DCHECK(false) << "Not implemented.";
}

bool NavigationManagerNewImpl::RemoveItemAtIndex(int index) {
  DCHECK(false) << "Not implemented.";
  return false;
}

void NavigationManagerNewImpl::CopyStateFromAndPrune(
    const NavigationManager* source) {
  DCHECK(false) << "Not implemented.";
}

bool NavigationManagerNewImpl::CanPruneAllButLastCommittedItem() const {
  // Used by BrowserViewController |loadURL| in OnAutocompleteAccept code path.
  // TODO(danyao): figure out how to fix this
  // DCHECK(false) << "Not implemented.";
  return true;
}

void NavigationManagerNewImpl::LoadURLWithParams(
    const NavigationManager::WebLoadParams& params) {
  DCHECK(!(params.transition_type & ui::PAGE_TRANSITION_FORWARD_BACK));

  // TODO(danyao): add support for native view and web UI.
  // Alternatively, this should point to a cleaner version of navigation stack
  // in CRWWebController

  // Bad, bad bad!! Don't call CRWWebController. This probably means that the
  // WKWebView interaction should be encapsulated in CRWWebController.
  [delegate_->GetWebController() ensureWebViewCreated];

  WKWebView* web_view = delegate_->GetWKWebView();
  DCHECK(web_view);

  // This block is copied from CRWWebController
  // |requestForCurrentNavigationItem|
  // TODO(danyao): move this into NavigationManager
  NSMutableURLRequest* request =
      [NSMutableURLRequest requestWithURL:net::NSURLWithGURL(params.url)];
  if (params.referrer.url.is_valid()) {
    std::string referrerValue =
        web::ReferrerHeaderValueForNavigation(params.url, params.referrer);
    if (!referrerValue.empty()) {
      [request setValue:base::SysUTF8ToNSString(referrerValue)
          forHTTPHeaderField:kReferrerHeaderName];
    }
  }

  // If there are headers in the current session entry add them to |request|.
  // Headers that would overwrite fields already present in |request| are
  // skipped.
  NSDictionary* headers = params.extra_headers;
  for (NSString* headerName in headers) {
    if (![request valueForHTTPHeaderField:headerName]) {
      [request setValue:[headers objectForKey:headerName]
          forHTTPHeaderField:headerName];
    }
  }

  // TODO(danyao): returns a WKNavigation that is not used.
  [web_view loadRequest:request];
}

void NavigationManagerNewImpl::GoBack() {
  [delegate_->GetWKWebView() goBack];
}

void NavigationManagerNewImpl::GoForward() {
  [delegate_->GetWKWebView() goForward];
}

void NavigationManagerNewImpl::GoToIndex(int index) {
  DCHECK(false) << "Not implemented";
  /*
  WKWebView* web_view = delegate_->GetWKWebView();
  WKBackForwardList* back_forward_list = [web_view backForwardList];
  // This is the wrong call because |itemAtIndex| expects its paramter to be
  relative to current item WKBackForwardListItem* item = [back_forward_list
  itemAtIndex:(NSInteger)index]; DCHECK(item); [web_view
  goToBackForwardListItem:item];*/
}

void NavigationManagerNewImpl::Reload(ReloadType reload_type,
                                      bool check_for_reposts) {
  [delegate_->GetWKWebView() reload];
}

void NavigationManagerNewImpl::AddTransientURLRewriter(
    BrowserURLRewriter::URLRewriter rewriter) {
  DCHECK(false) << "Not implemented.";
}

// Called to reset the transient url rewriter list.
void NavigationManagerNewImpl::RemoveTransientURLRewriters() {
  DCHECK(false) << "Not implemented.";
}

NavigationItem* NavigationManagerNewImpl::makeNavigationItem(
    const WKBackForwardListItem* current_item) const {
  if (current_item) {
    std::unique_ptr<NavigationItem> item = NavigationItem::Create();
    item->SetTitle(base::SysNSStringToUTF16([current_item title]));
    item->SetOriginalRequestURL(net::GURLWithNSURL([current_item initialURL]));
    item->SetURL(net::GURLWithNSURL([current_item URL]));

    navigation_item_holder_.push_back(std::move(item));
    return navigation_item_holder_.back().get();
  } else {
    return nullptr;
  }
}

/*NSString* NavigationManagerNewImpl::serializeSessionHistoryAsJson() const {
  NSData* json = [NSJSONSerialization dataWithJSONObject:restoredSessionHistory
options:NSJSONWritingPrettyPrinted error:nil]; NSString* encoded = [[NSString
alloc] initWithData:json encoding:NSUTF8StringEncoding]; return encoded;
}*/

void NavigationManagerNewImpl::RestoreSession() {
  // TODO(danyao): figure out a better place for this resource file. Right now
  // it is in ios/chrome/app, which will probably break all other apps that
  // use web layer directly.
  if (history_is_restored) {
    NSArray* parts = @[
      @"file://",
      [base::mac::FrameworkBundle() pathForResource:@"RestoreSession"
                                             ofType:@"html"],
      @"?sessionHistory=", base::SysUTF8ToNSString(restoredSessionHistory)
    ];
    NSURL* nsUrl = [NSURL URLWithString:[parts componentsJoinedByString:@""]];
    NSURLRequest* request = [NSURLRequest requestWithURL:nsUrl];
    DLOG(WARNING) << "File is "
                  << base::SysNSStringToUTF8(nsUrl.absoluteString);

    [delegate_->GetWebController() ensureWebViewCreated];
    WKWebView* web_view = delegate_->GetWKWebView();
    DCHECK(web_view);
    [web_view loadRequest:request];
  }
}

}  // namespace web
