// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_TOOLBAR_CLEAN_TOOLBAR_BUTTON_FACTORY_H_
#define IOS_CHROME_BROWSER_UI_TOOLBAR_CLEAN_TOOLBAR_BUTTON_FACTORY_H_

#import <UIKit/UIKit.h>

#import "ios/chrome/browser/ui/toolbar/clean/toolbar_style.h"

@protocol ApplicationCommands;
@protocol BrowserCommands;
@class ToolbarButton;
@class ToolbarToolsMenuButton;
@class ToolbarConfiguration;

// ToolbarButton Factory protocol to create ToolbarButton objects with certain
// style and configuration, depending of the implementation.
@interface ToolbarButtonFactory : NSObject

- (instancetype)initWithStyle:(ToolbarStyle)style NS_DESIGNATED_INITIALIZER;
- (instancetype)init NS_UNAVAILABLE;

@property(nonatomic, assign, readonly) ToolbarStyle style;
// Configuration object for styling. It is used by the factory to set the style
// of the buttons title.
@property(nonatomic, strong, readonly)
    ToolbarConfiguration* toolbarConfiguration;

// Back ToolbarButton.
- (ToolbarButton*)backButtonWithDispatcher:
    (id<ApplicationCommands, BrowserCommands>)dispatcher;
// Forward ToolbarButton.
- (ToolbarButton*)forwardButtonWithDispatcher:
    (id<ApplicationCommands, BrowserCommands>)dispatcher;
// Tab Switcher Strip ToolbarButton.
- (ToolbarButton*)tabSwitcherStripButtonWithDispatcher:
    (id<ApplicationCommands, BrowserCommands>)dispatcher;
// Tab Switcher Grid ToolbarButton.
- (ToolbarButton*)tabSwitcherGridButtonWithDispatcher:
    (id<ApplicationCommands, BrowserCommands>)dispatcher;
// Tools Menu ToolbarButton.
- (ToolbarToolsMenuButton*)toolsMenuButtonWithDispatcher:
    (id<ApplicationCommands, BrowserCommands>)dispatcher;
// Share ToolbarButton.
- (ToolbarButton*)shareButtonWithDispatcher:
    (id<ApplicationCommands, BrowserCommands>)dispatcher;
// Reload ToolbarButton.
- (ToolbarButton*)reloadButtonWithDispatcher:
    (id<ApplicationCommands, BrowserCommands>)dispatcher;
// Stop ToolbarButton.
- (ToolbarButton*)stopButtonWithDispatcher:
    (id<ApplicationCommands, BrowserCommands>)dispatcher;
// Bookmark ToolbarButton.
- (ToolbarButton*)bookmarkButtonWithDispatcher:
    (id<ApplicationCommands, BrowserCommands>)dispatcher;
// VoiceSearch ToolbarButton.
- (ToolbarButton*)voiceSearchButtonWithDispatcher:
    (id<ApplicationCommands, BrowserCommands>)dispatcher;
// ContractToolbar ToolbarButton.
- (ToolbarButton*)contractButtonWithDispatcher:
    (id<ApplicationCommands, BrowserCommands>)dispatcher;

// Returns images for Voice Search in an array representing the NORMAL/PRESSED
// state
- (NSArray<UIImage*>*)voiceSearchImages;
// Returns images for TTS in an array representing the NORMAL/PRESSED states.
- (NSArray<UIImage*>*)TTSImages;

@end

#endif  // IOS_CHROME_BROWSER_UI_TOOLBAR_CLEAN_TOOLBAR_BUTTON_FACTORY_H_
