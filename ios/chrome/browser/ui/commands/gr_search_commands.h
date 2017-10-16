// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_COMMANDS_GR_SEARCH_COMMANDS_H_
#define IOS_CHROME_BROWSER_UI_COMMANDS_GR_SEARCH_COMMANDS_H_

#import <Foundation/Foundation.h>

// This protocol groups commands related to GR search.
@protocol GRSearchCommands

// Lauches the GR search UI.
- (void)launchGRSearch;

@end

#endif  // IOS_CHROME_BROWSER_UI_COMMANDS_GR_SEARCH_COMMANDS_H_
