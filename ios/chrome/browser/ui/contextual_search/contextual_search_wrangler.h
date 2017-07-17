// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_CONTEXTUAL_SEARCH_CONTEXTUAL_SEARCH_WRANGLER_H_
#define IOS_CHROME_BROWSER_UI_CONTEXTUAL_SEARCH_CONTEXTUAL_SEARCH_WRANGLER_H_

#import <UIKit/UIKit.h>

#import "ios/chrome/browser/ui/contextual_search/contextual_search_controller.h"
#import "ios/chrome/browser/ui/contextual_search/contextual_search_panel_protocols.h"

@class TabModel;
namespace ios {
class ChromeBrowserState;
}

@protocol ContextualSearchProvider<NSObject>

@property(nonatomic, readonly) UIView* view;

@property(nonatomic, readonly) UIView* tabStripView;

@property(nonatomic, readonly) UIView* toolbarView;

- (void)updateToolbarControlsAlpha:(CGFloat)alpha;

- (void)updateToolbarBackgroundAlpha:(CGFloat)alpha;

- (void)tabLoadComplete:(Tab*)tab withSuccess:(BOOL)success;

- (void)focusOmnibox;

- (void)restoreInfobars;

- (void)suspendInfobars;

- (CGFloat)currentHeaderOffset;

@end

@interface ContextualSearchWrangler
    : NSObject<ContextualSearchControllerDelegate,
               ContextualSearchPanelMotionObserver>

- (instancetype)initWithProvider:(id<ContextualSearchProvider>)provider
                        tabModel:(TabModel*)tabModel NS_DESIGNATED_INITIALIZER;

- (instancetype)init NS_UNAVAILABLE;

- (void)enable:(BOOL)enabled;

- (void)insertPanelView;

- (void)maybeStartForBrowserState:(ios::ChromeBrowserState*)browserState;

- (void)stop;

@end

#endif  // IOS_CHROME_BROWSER_UI_CONTEXTUAL_SEARCH_CONTEXTUAL_SEARCH_WRANGLER_H_
