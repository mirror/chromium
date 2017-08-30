// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CLEAN_CHROME_BROWSER_UI_DIALOGS_CONTEXT_MENU_CONTEXT_MENU_OVERLAY_COORDINATOR_H_
#define IOS_CLEAN_CHROME_BROWSER_UI_DIALOGS_CONTEXT_MENU_CONTEXT_MENU_OVERLAY_COORDINATOR_H_

#import <Foundation/Foundation.h>

#import "ios/clean/chrome/browser/ui/dialogs/dialog_coordinator.h"

@class ContextMenuRequest;

// An overlay coordinator for the UI elements used to present context menus.
// TODO: Name was chosen to avoid collisions with legacy context menu
// implementation; this class should be renamed when moving to new architecture.
@interface ContextMenuOverlayCoordinator : DialogCoordinator

// Designated initializer to display a context menu with |request|.
- (instancetype)initWithRequest:(ContextMenuRequest*)request
    NS_DESIGNATED_INITIALIZER;
- (instancetype)init NS_UNAVAILABLE;

@end

#endif  // IOS_CLEAN_CHROME_BROWSER_UI_DIALOGS_CONTEXT_MENU_CONTEXT_MENU_OVERLAY_COORDINATOR_H_
