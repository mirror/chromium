// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CLEAN_CHROME_BROWSER_UI_COMMANDS_RECENT_TABS_COMMANDS_H_
#define IOS_CLEAN_CHROME_BROWSER_UI_COMMANDS_RECENT_TABS_COMMANDS_H_

// Commands relating to the Recent Tabs UI.
@protocol RecentTabsCommands
// Dismiss the Recent Tabs UI.
- (void)closeRecentTabs;
@end

#endif  // IOS_CLEAN_CHROME_BROWSER_UI_COMMANDS_RECENT_TABS_COMMANDS_H_
