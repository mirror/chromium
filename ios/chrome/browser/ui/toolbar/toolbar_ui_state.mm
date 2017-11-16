// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/toolbar/toolbar_ui_state.h"

#include <memory>

#include "base/logging.h"
#include "base/memory/ptr_util.h"
#import "ios/chrome/browser/ui/toolbar/toolbar_owner.h"
#import "ios/chrome/browser/web_state_list/web_state_list.h"
#import "ios/chrome/browser/web_state_list/web_state_list_observer_bridge.h"
#import "ios/web/public/web_state/navigation_context.h"
#import "ios/web/public/web_state/web_state.h"
#import "ios/web/public/web_state/web_state_observer_bridge.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

#pragma mark - ToolbarUIState

@interface ToolbarUIState ()
// Redefine property as readwrite.
@property(nonatomic, assign) CGFloat toolbarHeight;
@end

@implementation ToolbarUIState
@synthesize toolbarHeight = _toolbarHeight;
@end

#pragma mark - ToolbarUIStateUpdater

@interface ToolbarUIStateUpdater ()<CRWWebStateObserver,
                                    WebStateListObserving> {
  // The bridge for WebStateList observation.
  std::unique_ptr<WebStateListObserverBridge> _webStateListObserver;
  // The bridge for WebState observation.
  std::unique_ptr<web::WebStateObserverBridge> _webStateObserver;
}
// The ToolbarOwner passed on initialization.
@property(nonatomic, readonly, strong) id<ToolbarOwner> toolbarOwner;
// The WebStateList whose navigations are driving this updater.
@property(nonatomic, readonly) WebStateList* webStateList;
// The active WebState in |webStateList|.
@property(nonatomic, assign) web::WebState* webState;

// Updates |state| using |toolbarOwner|.
- (void)updateState;

@end

@implementation ToolbarUIStateUpdater
@synthesize state = _state;
@synthesize toolbarOwner = _toolbarOwner;
@synthesize webStateList = _webStateList;
@synthesize webState = _webState;

- (instancetype)initWithState:(ToolbarUIState*)state
                 toolbarOwner:(id<ToolbarOwner>)toolbarOwner
                 webStateList:(WebStateList*)webStateList {
  if (self = [super init]) {
    _state = state;
    DCHECK(_state);
    _toolbarOwner = toolbarOwner;
    DCHECK(_toolbarOwner);
    _webStateList = webStateList;
    DCHECK(_webStateList);
  }
  return self;
}

#pragma mark Accessors

- (void)setWebState:(web::WebState*)webState {
  if (_webState == webState)
    return;
  if (_webState) {
    DCHECK(_webStateObserver);
    _webState->RemoveObserver(_webStateObserver.get());
    _webStateObserver = nullptr;
  }
  _webState = webState;
  if (_webState) {
    _webStateObserver = base::MakeUnique<web::WebStateObserverBridge>(self);
    _webState->AddObserver(_webStateObserver.get());
    [self updateState];
  }
}

#pragma mark Public

- (void)startUpdating {
  DCHECK(!_webStateListObserver);
  DCHECK(!_webStateObserver);
  _webStateListObserver = base::MakeUnique<WebStateListObserverBridge>(self);
  self.webStateList->AddObserver(_webStateListObserver.get());
  self.webState = self.webStateList->GetActiveWebState();
  [self updateState];
}

- (void)stopUpdating {
  DCHECK(_webStateListObserver);
  DCHECK(!self.webState || _webStateObserver);
  self.webStateList->RemoveObserver(_webStateListObserver.get());
  self.webState = nullptr;
}

#pragma mark CRWWebStateObserver

- (void)webState:(web::WebState*)webState
    didCommitNavigationWithDetails:
        (const web::LoadCommittedDetails&)load_details {
  [self updateState];
}

- (void)webState:(web::WebState*)webState
    didStartNavigation:(web::NavigationContext*)navigation {
  // For user-initiated loads, the toolbar is updated when the navigation is
  // started.
  if (!navigation->IsRendererInitiated())
    [self updateState];
}

#pragma mark WebStateListObserving

- (void)webStateList:(WebStateList*)webStateList
    didChangeActiveWebState:(web::WebState*)newWebState
                oldWebState:(web::WebState*)oldWebState
                    atIndex:(int)atIndex
                 userAction:(BOOL)userAction {
  DCHECK_EQ(self.webStateList, webStateList);
  self.webState = newWebState;
}

#pragma mark Private

- (void)updateState {
  self.state.toolbarHeight = [self.toolbarOwner toolbarHeight];
}

@end
