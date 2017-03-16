// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/web/navigation/crw_session_controller.h"

#include <stddef.h>

#include <algorithm>
#include <utility>

#include "base/format_macros.h"
#include "base/logging.h"
#import "base/mac/foundation_util.h"
#import "base/mac/scoped_nsobject.h"
#include "base/memory/ptr_util.h"
#include "base/strings/sys_string_conversions.h"
#include "ios/web/history_state_util.h"
#import "ios/web/navigation/crw_session_certificate_policy_manager.h"
#import "ios/web/navigation/crw_session_controller+private_constructors.h"
#import "ios/web/navigation/navigation_item_impl.h"
#include "ios/web/navigation/navigation_manager_facade_delegate.h"
#import "ios/web/navigation/navigation_manager_impl.h"
#include "ios/web/navigation/time_smoother.h"
#include "ios/web/public/browser_state.h"
#include "ios/web/public/browser_url_rewriter.h"
#include "ios/web/public/referrer.h"
#include "ios/web/public/ssl_status.h"
#import "ios/web/public/web_client.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@interface CRWSessionController () {
  // Weak pointer back to the owning NavigationManager. This is to facilitate
  // the incremental merging of the two classes.
  web::NavigationManagerImpl* _navigationManager;

  // Identifies the index of the current navigation in the NavigationItem
  // array.
  NSInteger _currentNavigationIndex;
  // Identifies the index of the previous navigation in the NavigationItem
  // array.
  NSInteger _previousNavigationIndex;

   // Stores the certificate policies decided by the user.
  CRWSessionCertificatePolicyManager* _sessionCertificatePolicyManager;

  // The browser state associated with this CRWSessionController;
  web::BrowserState* _browserState;  // weak

  // Time smoother for navigation item timestamps; see comment in
  // navigation_controller_impl.h
  web::TimeSmoother _timeSmoother;

  // Backing objects for properties of the same name.
  web::ScopedNavigationItemImplList _items;
  // |_pendingItem| only contains a NavigationItem for non-history navigations.
  // For back/forward navigations within session history, _pendingItemIndex will
  // be equal to -1, and self.pendingItem will return an item contained within
  // |_items|.
  std::unique_ptr<web::NavigationItemImpl> _pendingItem;
  std::unique_ptr<web::NavigationItemImpl> _transientItem;
}

// Redefine as readwrite.
@property(nonatomic, readwrite, assign) NSInteger currentNavigationIndex;

// TODO(rohitrao): These properties must be redefined readwrite to work around a
// clang bug. crbug.com/228650
@property(nonatomic, readwrite, strong)
    CRWSessionCertificatePolicyManager* sessionCertificatePolicyManager;

// Expose setters for serialization properties.  These are exposed in a category
// in SessionStorageBuilder, and will be removed as ownership of
// their backing ivars moves to NavigationManagerImpl.
@property(nonatomic, readwrite, getter=isOpenedByDOM) BOOL openedByDOM;
@property(nonatomic, readwrite, assign) NSInteger previousNavigationIndex;

// Removes all items after currentNavigationIndex_.
- (void)clearForwardItems;
// Discards the transient item, if any.
- (void)discardTransientItem;
// Creates a NavigationItemImpl with the specified properties.
- (std::unique_ptr<web::NavigationItemImpl>)
   itemWithURL:(const GURL&)url
      referrer:(const web::Referrer&)referrer
    transition:(ui::PageTransition)transition
initiationType:(web::NavigationInitiationType)initiationType;
// Returns YES if the PageTransition for the underlying navigationItem at
// |index| in |items| has ui::PAGE_TRANSITION_IS_REDIRECT_MASK.
- (BOOL)isRedirectTransitionForItemAtIndex:(size_t)index;

@end

@implementation CRWSessionController

@synthesize currentNavigationIndex = _currentNavigationIndex;
@synthesize previousNavigationIndex = _previousNavigationIndex;
@synthesize pendingItemIndex = _pendingItemIndex;
@synthesize openedByDOM = _openedByDOM;
@synthesize sessionCertificatePolicyManager = _sessionCertificatePolicyManager;

- (instancetype)initWithBrowserState:(web::BrowserState*)browserState
                         openedByDOM:(BOOL)openedByDOM {
  self = [super init];
  if (self) {
    _openedByDOM = openedByDOM;
    _browserState = browserState;
    _currentNavigationIndex = -1;
    _previousNavigationIndex = -1;
    _pendingItemIndex = -1;
    _sessionCertificatePolicyManager =
        [[CRWSessionCertificatePolicyManager alloc] init];
  }
  return self;
}

- (instancetype)initWithBrowserState:(web::BrowserState*)browserState
                     navigationItems:(web::ScopedNavigationItemList)items
                        currentIndex:(NSUInteger)currentIndex {
  self = [super init];
  if (self) {
    _browserState = browserState;
    _items = web::CreateScopedNavigationItemImplList(std::move(items));
    _currentNavigationIndex =
        std::min(static_cast<NSInteger>(currentIndex),
                 static_cast<NSInteger>(_items.size()) - 1);
    _previousNavigationIndex = -1;
    _pendingItemIndex = -1;
    _sessionCertificatePolicyManager =
        [[CRWSessionCertificatePolicyManager alloc] init];
  }
  return self;
}

#pragma mark - Accessors

- (void)setCurrentNavigationIndex:(NSInteger)currentNavigationIndex {
  if (_currentNavigationIndex != currentNavigationIndex) {
    _currentNavigationIndex = currentNavigationIndex;
    if (_navigationManager)
      _navigationManager->RemoveTransientURLRewriters();
  }
}

- (void)setPendingItemIndex:(NSInteger)pendingItemIndex {
  DCHECK_GE(pendingItemIndex, -1);
  DCHECK_LT(pendingItemIndex, static_cast<NSInteger>(self.items.size()));
  _pendingItemIndex = pendingItemIndex;
  DCHECK(_pendingItemIndex == -1 || self.pendingItem);
}

- (BOOL)canPruneAllButLastCommittedItem {
  return self.currentNavigationIndex != -1 && self.pendingItemIndex == -1 &&
         !self.transientItem;
}

- (const web::ScopedNavigationItemImplList&)items {
  return _items;
}

- (web::NavigationItemImpl*)currentItem {
  if (self.transientItem)
    return self.transientItem;
  if (self.pendingItem)
    return self.pendingItem;
  return self.lastCommittedItem;
}

- (web::NavigationItemImpl*)visibleItem {
  if (self.transientItem)
    return self.transientItem;
  // Only return the |pendingItem| for new (non-history), browser-initiated
  // navigations in order to prevent URL spoof attacks.
  web::NavigationItemImpl* pendingItem = self.pendingItem;
  if (pendingItem) {
    bool isUserInitiated = pendingItem->NavigationInitiationType() ==
                           web::NavigationInitiationType::USER_INITIATED;
    bool safeToShowPending = isUserInitiated && _pendingItemIndex == -1;

    if (safeToShowPending)
      return pendingItem;
  }
  return self.lastCommittedItem;
}

- (web::NavigationItemImpl*)pendingItem {
  if (self.pendingItemIndex == -1)
    return _pendingItem.get();
  return self.items[self.pendingItemIndex].get();
}

- (web::NavigationItemImpl*)transientItem {
  return _transientItem.get();
}

- (web::NavigationItemImpl*)lastCommittedItem {
  NSInteger index = self.currentNavigationIndex;
  return index == -1 ? nullptr : self.items[index].get();
}

- (web::NavigationItemImpl*)previousItem {
  NSInteger index = self.previousNavigationIndex;
  return index == -1 || self.items.empty() ? nullptr : self.items[index].get();
}

- (web::NavigationItemImpl*)lastUserItem {
  if (self.items.empty())
    return nil;

  NSInteger index = self.currentNavigationIndex;
  // This will return the first NavigationItem if all other items are
  // redirects, regardless of the transition state of the first item.
  while (index > 0 && [self isRedirectTransitionForItemAtIndex:index])
    --index;

  return self.items[index].get();
}

- (web::NavigationItemList)backwardItems {
  web::NavigationItemList items;
  for (size_t index = _currentNavigationIndex; index > 0; --index) {
    if (![self isRedirectTransitionForItemAtIndex:index])
      items.push_back(self.items[index - 1].get());
  }
  return items;
}

- (web::NavigationItemList)forwardItems {
  web::NavigationItemList items;
  NSUInteger lastNonRedirectedIndex = _currentNavigationIndex + 1;
  while (lastNonRedirectedIndex < self.items.size()) {
    web::NavigationItem* item = self.items[lastNonRedirectedIndex].get();
    if (!ui::PageTransitionIsRedirect(item->GetTransitionType()))
      items.push_back(item);
    ++lastNonRedirectedIndex;
  }
  return items;
}

#pragma mark - NSObject

- (NSString*)description {
  // Create description for |items|.
  NSMutableString* itemsDescription = [NSMutableString stringWithString:@"[\n"];
#ifndef NDEBUG
  for (const auto& item : self.items)
    [itemsDescription appendFormat:@"%@\n", item->GetDescription()];
#endif
  [itemsDescription appendString:@"]"];
  // Create description for |pendingItem| and |transientItem|.
  NSString* pendingItemDescription = @"(null)";
  NSString* transientItemDescription = @"(null)";
#ifndef NDEBUG
  if (self.pendingItem)
    pendingItemDescription = self.pendingItem->GetDescription();
  if (self.transientItem)
    transientItemDescription = self.transientItem->GetDescription();
#else
  if (self.pendingItem) {
    pendingItemDescription =
        [NSString stringWithFormat:@"%p", self.pendingItem];
  }
  if (self.transientItem) {
    transientItemDescription =
        [NSString stringWithFormat:@"%p", self.transientItem];
  }
#endif
  return [NSString stringWithFormat:@"current index: %" PRIdNS
                                    @"\nprevious index: %" PRIdNS
                                    @"\npending"
                                    @" index: %" PRIdNS
                                    @"\n%@\npending: %@\ntransient: %@\n",
                                    _currentNavigationIndex,
                                    _previousNavigationIndex, _pendingItemIndex,
                                    itemsDescription, pendingItemDescription,
                                    transientItemDescription];
}

#pragma mark - Public

- (void)setNavigationManager:(web::NavigationManagerImpl*)navigationManager {
  _navigationManager = navigationManager;
  if (_navigationManager) {
    // _browserState will be nullptr if CRWSessionController has been
    // initialized with -initWithCoder: method. Take _browserState from
    // NavigationManagerImpl if that's the case.
    if (!_browserState) {
      _browserState = _navigationManager->GetBrowserState();
    }
    DCHECK_EQ(_browserState, _navigationManager->GetBrowserState());
  }
}

- (void)setBrowserState:(web::BrowserState*)browserState {
  _browserState = browserState;
  DCHECK(!_navigationManager ||
         _navigationManager->GetBrowserState() == _browserState);
}

- (void)addPendingItem:(const GURL&)url
              referrer:(const web::Referrer&)ref
            transition:(ui::PageTransition)trans
        initiationType:(web::NavigationInitiationType)initiationType {
  [self discardTransientItem];
  self.pendingItemIndex = -1;

  // Don't create a new item if it's already the same as the current item,
  // allowing this routine to be called multiple times in a row without issue.
  // Note: CRWSessionController currently has the responsibility to distinguish
  // between new navigations and history stack navigation, hence the inclusion
  // of specific transiton type logic here, in order to make it reliable with
  // real-world observed behavior.
  // TODO(crbug.com/676129): Fix the way changes are detected/reported elsewhere
  // in the web layer so that this hack can be removed.
  // Remove the workaround code from -presentSafeBrowsingWarningForResource:.
  web::NavigationItemImpl* currentItem = self.currentItem;
  if (currentItem) {
    BOOL hasSameURL = currentItem->GetURL() == url;
    BOOL isPendingTransitionFormSubmit =
        PageTransitionCoreTypeIs(trans, ui::PAGE_TRANSITION_FORM_SUBMIT);
    BOOL isCurrentTransitionFormSubmit = PageTransitionCoreTypeIs(
        currentItem->GetTransitionType(), ui::PAGE_TRANSITION_FORM_SUBMIT);
    BOOL shouldCreatePendingItem =
        !hasSameURL ||
        (isPendingTransitionFormSubmit && !isCurrentTransitionFormSubmit);

    if (!shouldCreatePendingItem) {
      // Send the notification anyway, to preserve old behavior. It's unknown
      // whether anything currently relies on this, but since both this whole
      // hack and the content facade will both be going away, it's not worth
      // trying to unwind.
      if (_navigationManager && _navigationManager->GetFacadeDelegate())
        _navigationManager->GetFacadeDelegate()->OnNavigationItemPending();
      return;
    }
  }

  _pendingItem = [self itemWithURL:url
                          referrer:ref
                        transition:trans
                    initiationType:initiationType];

  if (_navigationManager && _navigationManager->GetFacadeDelegate())
    _navigationManager->GetFacadeDelegate()->OnNavigationItemPending();
}

- (void)updatePendingItem:(const GURL&)url {
  // If there is no pending item, navigation is probably happening within the
  // session history. Don't modify the item list.
  web::NavigationItemImpl* item = self.pendingItem;
  if (!item)
    return;

  if (url != item->GetURL()) {
    // Assume a redirection, and discard any transient item.
    // TODO(stuartmorgan): Once the current safe browsing code is gone,
    // consider making this a DCHECK that there's no transient item.
    [self discardTransientItem];

    item->SetURL(url);
    item->SetVirtualURL(url);
    // Redirects (3xx response code), or client side navigation must change
    // POST requests to GETs.
    item->SetPostData(nil);
    item->ResetHttpRequestHeaders();
  }

  // This should probably not be sent if the URLs matched, but that's what was
  // done before, so preserve behavior in case something relies on it.
  if (_navigationManager && _navigationManager->GetFacadeDelegate())
    _navigationManager->GetFacadeDelegate()->OnNavigationItemPending();
}

- (void)clearForwardItems {
  DCHECK_EQ(self.pendingItemIndex, -1);
  [self discardTransientItem];

  NSInteger forwardItemStartIndex = _currentNavigationIndex + 1;
  DCHECK(forwardItemStartIndex >= 0);

  size_t itemCount = self.items.size();
  if (forwardItemStartIndex >= static_cast<NSInteger>(itemCount))
    return;

  if (_previousNavigationIndex >= forwardItemStartIndex)
    _previousNavigationIndex = -1;

  // Remove the NavigationItems and notify the NavigationManater
  _items.erase(_items.begin() + forwardItemStartIndex, _items.end());
  if (_navigationManager) {
    _navigationManager->OnNavigationItemsPruned(itemCount -
                                                forwardItemStartIndex);
  }
}

- (void)commitPendingItem {
  if (self.pendingItem) {
    // Once an item is committed it's not renderer-initiated any more. (Matches
    // the implementation in NavigationController.)
    self.pendingItem->ResetForCommit();

    NSInteger newNavigationIndex = self.pendingItemIndex;
    if (newNavigationIndex == -1) {
      [self clearForwardItems];
      // Add the new item at the end.
      _items.push_back(std::move(_pendingItem));
      newNavigationIndex = self.items.size() - 1;
    }
    _previousNavigationIndex = _currentNavigationIndex;
    self.currentNavigationIndex = newNavigationIndex;
    self.pendingItemIndex = -1;
    DCHECK(!_pendingItem);
  }

  web::NavigationItem* item = self.currentItem;
  // Update the navigation timestamp now that it's actually happened.
  if (item)
    item->SetTimestamp(_timeSmoother.GetSmoothedTime(base::Time::Now()));

  if (_navigationManager && item)
    _navigationManager->OnNavigationItemCommitted();
  DCHECK_EQ(self.pendingItemIndex, -1);
}

- (void)addTransientItemWithURL:(const GURL&)URL {
  _transientItem =
      [self itemWithURL:URL
                referrer:web::Referrer()
              transition:ui::PAGE_TRANSITION_CLIENT_REDIRECT
          initiationType:web::NavigationInitiationType::USER_INITIATED];
  _transientItem->SetTimestamp(
      _timeSmoother.GetSmoothedTime(base::Time::Now()));
}

- (void)pushNewItemWithURL:(const GURL&)URL
               stateObject:(NSString*)stateObject
                transition:(ui::PageTransition)transition {
  DCHECK(!self.pendingItem);
  DCHECK(self.currentItem);

  web::NavigationItem* lastCommittedItem = self.lastCommittedItem;
  CHECK(web::history_state_util::IsHistoryStateChangeValid(
      lastCommittedItem->GetURL(), URL));

  web::Referrer referrer(lastCommittedItem->GetURL(),
                         web::ReferrerPolicyDefault);
  std::unique_ptr<web::NavigationItemImpl> pushedItem =
      [self itemWithURL:URL
                referrer:referrer
              transition:transition
          initiationType:web::NavigationInitiationType::USER_INITIATED];
  pushedItem->SetUserAgentType(lastCommittedItem->GetUserAgentType());
  pushedItem->SetSerializedStateObject(stateObject);
  pushedItem->SetIsCreatedFromPushState(true);
  pushedItem->GetSSL() = lastCommittedItem->GetSSL();
  pushedItem->SetTimestamp(_timeSmoother.GetSmoothedTime(base::Time::Now()));

  [self clearForwardItems];
  // Add the new item at the end.
  _items.push_back(std::move(pushedItem));
  _previousNavigationIndex = _currentNavigationIndex;
  self.currentNavigationIndex = self.items.size() - 1;

  if (_navigationManager)
    _navigationManager->OnNavigationItemCommitted();
}

- (void)updateCurrentItemWithURL:(const GURL&)url
                     stateObject:(NSString*)stateObject {
  DCHECK(!self.transientItem);
  web::NavigationItemImpl* currentItem = self.currentItem;
  currentItem->SetURL(url);
  currentItem->SetSerializedStateObject(stateObject);
  currentItem->SetHasStateBeenReplaced(true);
  currentItem->SetPostData(nil);
  // If the change is to a committed item, notify interested parties.
  if (currentItem != self.pendingItem && _navigationManager)
    _navigationManager->OnNavigationItemChanged();
}

- (void)discardNonCommittedItems {
  [self discardTransientItem];
  _pendingItem.reset();
  self.pendingItemIndex = -1;
}

- (void)discardTransientItem {
  _transientItem.reset();
}

- (void)copyStateFromSessionControllerAndPrune:(CRWSessionController*)source {
  DCHECK(source);
  if (!self.canPruneAllButLastCommittedItem)
    return;

  // The other session may not have any items, in which case there is nothing
  // to insert.
  const web::ScopedNavigationItemImplList& sourceItems = source->_items;
  if (sourceItems.empty())
    return;

  // Early return if there's no committed source item.
  if (!source.lastCommittedItem)
    return;

  // Copy |sourceItems| into a new NavigationItemList.  |mergedItems| is needs
  // to be large enough for all items in |source| preceding
  // |sourceCurrentIndex|, the |source|'s current item, and |self|'s current
  // item, which comes out to |sourceCurrentIndex| + 2.
  DCHECK_GT(source.currentNavigationIndex, -1);
  size_t sourceCurrentIndex =
      static_cast<size_t>(source.currentNavigationIndex);
  web::ScopedNavigationItemImplList mergedItems(sourceCurrentIndex + 2);
  for (size_t index = 0; index <= sourceCurrentIndex; ++index) {
    mergedItems[index] =
        base::MakeUnique<web::NavigationItemImpl>(*sourceItems[index]);
  }
  mergedItems.back() = std::move(_items[self.currentNavigationIndex]);

  // Use |mergedItems| as the session history.
  std::swap(mergedItems, _items);

  // Update state to reflect inserted NavigationItems.
  _previousNavigationIndex = -1;
  _currentNavigationIndex = self.items.size() - 1;

  DCHECK_LT(static_cast<NSUInteger>(_currentNavigationIndex),
            self.items.size());
}

- (void)goToItemAtIndex:(NSInteger)index {
  if (index < 0 || static_cast<NSUInteger>(index) >= self.items.size())
    return;

  if (index < _currentNavigationIndex) {
    // Going back.
    [self discardNonCommittedItems];
  } else if (_currentNavigationIndex < index) {
    // Going forward.
    [self discardTransientItem];
  } else {
    // |delta| is 0, no need to change current navigation index.
    return;
  }

  _previousNavigationIndex = _currentNavigationIndex;
  _currentNavigationIndex = index;
}

- (void)removeItemAtIndex:(NSInteger)index {
  DCHECK(index < static_cast<NSInteger>(self.items.size()));
  DCHECK(index != _currentNavigationIndex);
  DCHECK(index >= 0);

  [self discardNonCommittedItems];

  _items.erase(_items.begin() + index);
  if (_currentNavigationIndex > index)
    _currentNavigationIndex--;
  if (_previousNavigationIndex >= index)
    _previousNavigationIndex--;
}

- (BOOL)isSameDocumentNavigationBetweenItem:(web::NavigationItem*)firstItem
                                    andItem:(web::NavigationItem*)secondItem {
  if (!firstItem || !secondItem || firstItem == secondItem)
    return NO;
  NSUInteger firstIndex = [self indexOfItem:firstItem];
  NSUInteger secondIndex = [self indexOfItem:secondItem];
  if (firstIndex == NSNotFound || secondIndex == NSNotFound)
    return NO;
  NSUInteger startIndex = firstIndex < secondIndex ? firstIndex : secondIndex;
  NSUInteger endIndex = firstIndex < secondIndex ? secondIndex : firstIndex;

  for (NSUInteger i = startIndex + 1; i <= endIndex; i++) {
    web::NavigationItemImpl* item = self.items[i].get();
    // Every item in the sequence has to be created from a hash change or
    // pushState() call.
    if (!item->IsCreatedFromPushState() && !item->IsCreatedFromHashChange())
      return NO;
    // Every item in the sequence has to have a URL that could have been
    // created from a pushState() call.
    if (!web::history_state_util::IsHistoryStateChangeValid(firstItem->GetURL(),
                                                            item->GetURL()))
      return NO;
  }
  return YES;
}

- (NSInteger)indexOfItem:(const web::NavigationItem*)item {
  DCHECK(item);
  for (size_t index = 0; index < self.items.size(); ++index) {
    if (self.items[index].get() == item)
      return index;
  }
  return NSNotFound;
}

- (web::NavigationItemImpl*)itemAtIndex:(NSInteger)index {
  if (index < 0 || self.items.size() <= static_cast<NSUInteger>(index))
    return nullptr;
  return self.items[index].get();
}

#pragma mark -
#pragma mark Private methods

- (std::unique_ptr<web::NavigationItemImpl>)
   itemWithURL:(const GURL&)url
      referrer:(const web::Referrer&)referrer
    transition:(ui::PageTransition)transition
initiationType:(web::NavigationInitiationType)initiationType {
  GURL loaded_url(url);
  BOOL urlWasRewritten = NO;
  if (_navigationManager) {
    std::unique_ptr<std::vector<web::BrowserURLRewriter::URLRewriter>>
        transientRewriters = _navigationManager->GetTransientURLRewriters();
    if (transientRewriters) {
      urlWasRewritten = web::BrowserURLRewriter::RewriteURLWithWriters(
          &loaded_url, _browserState, *transientRewriters.get());
    }
  }
  if (!urlWasRewritten) {
    web::BrowserURLRewriter::GetInstance()->RewriteURLIfNecessary(
        &loaded_url, _browserState);
  }

  std::unique_ptr<web::NavigationItemImpl> item(new web::NavigationItemImpl());
  item->SetOriginalRequestURL(loaded_url);
  item->SetURL(loaded_url);
  item->SetReferrer(referrer);
  item->SetTransitionType(transition);
  item->SetNavigationInitiationType(initiationType);
  if (web::GetWebClient()->IsAppSpecificURL(loaded_url))
    item->SetUserAgentType(web::UserAgentType::NONE);
  return item;
}

- (BOOL)isRedirectTransitionForItemAtIndex:(size_t)index {
  DCHECK_LT(index, self.items.size());
  ui::PageTransition transition = self.items[index]->GetTransitionType();
  return (transition & ui::PAGE_TRANSITION_IS_REDIRECT_MASK) ? YES : NO;
}

@end
