// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CLEAN_CHROME_BROWSER_UI_COMMANDS_CONTEXT_MENU_COMMANDS_H_
#define IOS_CLEAN_CHROME_BROWSER_UI_COMMANDS_CONTEXT_MENU_COMMANDS_H_

@class ContextMenuRequest;

// Commands relating to the context menu.
@protocol ContextMenuCommands<NSObject>
@optional
// Executes |request|'s script.
- (void)executeContextMenuScript:(ContextMenuRequest*)request;
// Opens |request|'s link in a new tab.
- (void)openContextMenuLinkInNewTab:(ContextMenuRequest*)request;
// Opens |request|'s link in a new Incognito tab.
- (void)openContextMenuLinkInNewIncognitoTab:(ContextMenuRequest*)request;
// Copies |request|'s link to the paste board.
- (void)copyContextMenuLink:(ContextMenuRequest*)request;
// Saves's the image at |request|'s image URL to the camera roll.
- (void)saveContextMenuImage:(ContextMenuRequest*)request;
// Opens the image at |request|'s image URL .
- (void)openContextMenuImage:(ContextMenuRequest*)request;
// Opens the image at |request|'s image URL  in a new tab.
- (void)openContextMenuImageInNewTab:(ContextMenuRequest*)request;
@end

// Command protocol for dismissing context menus.
@protocol ContextMenuDismissalCommands<NSObject>

// Called after the user interaction with the context menu has been handled
// and the UI can be dismissed.
- (void)dismissContextMenu;

// Called after the user has selected an unavailable feature.
// TODO: Delete after all context menu commands are implemented.
- (void)dismissContextMenuForUnavailableFeatureWithName:(NSString*)featureName;

@end

#endif  // IOS_CLEAN_CHROME_BROWSER_UI_COMMANDS_CONTEXT_MENU_COMMANDS_H_
