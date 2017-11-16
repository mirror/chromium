// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_TOOLBAR_TOOLBAR_UI_STATE_H_
#define IOS_CHROME_BROWSER_UI_TOOLBAR_TOOLBAR_UI_STATE_H_

#import <UIKit/UIKit.h>

@protocol ToolbarOwner;
class WebStateList;

// An object encapsulating the broadcasted state of the toolbar.
@interface ToolbarUIState : NSObject

// The height of the toolbar, not including the safe area inset.
// This should be broadcast using |-broadcastToolbarHeight:|.
@property(nonatomic, readonly) CGFloat toolbarHeight;

@end

// Helper object that uses navigation events to update a ToolbarUIState.
@interface ToolbarUIStateUpdater : NSObject

// The state object passed on initialization.  It's being updated by this
// object.
@property(nonatomic, strong, readonly, nonnull) ToolbarUIState* state;

// Designated initializer that uses navigation events from |webStateList| and
// the height provided by |toolbarOwner| to update |state|'s broadcast value.
- (nullable instancetype)initWithState:(nonnull ToolbarUIState*)state
                          toolbarOwner:(nonnull id<ToolbarOwner>)toolbarOwner
                          webStateList:(nonnull WebStateList*)webStateList
    NS_DESIGNATED_INITIALIZER;
- (nullable instancetype)init NS_UNAVAILABLE;

// Starts updating |state|.
- (void)startUpdating;

// Stops updating |state|.
- (void)stopUpdating;

@end

#endif  // IOS_CHROME_BROWSER_UI_TOOLBAR_TOOLBAR_UI_STATE_H_
