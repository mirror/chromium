// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_COMMANDS_START_VOICE_SEARCH_COMMAND_H_
#define IOS_CHROME_BROWSER_UI_COMMANDS_START_VOICE_SEARCH_COMMAND_H_

#import <UIKit/UIKit.h>

#import "ios/chrome/browser/ui/commands/generic_chrome_command.h"

class ViewInfo;

// Command sent to start a voice search, optionally including the view from
// which the voice search present and dismiss animations will occur.
@interface StartVoiceSearchCommand : GenericChromeCommand

// Designated initializer for a command to start voice search using
// |originViewInfo|'s center as the start point for transition animations.
- (instancetype)initWithOriginView:(UIView*)originView
    NS_DESIGNATED_INITIALIZER;
- (instancetype)initWithTag:(NSInteger)tag NS_UNAVAILABLE;

// The view info passed on initialization.
@property(nonatomic, readonly) const ViewInfo* originViewInfo;

@end

#endif  // IOS_CHROME_BROWSER_UI_COMMANDS_START_VOICE_SEARCH_COMMAND_H_
