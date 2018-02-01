// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import <Foundation/Foundation.h>

// Protocol used by RecentTabsTableViewController to communicate to its
// Coordinator.
@protocol RecentTabsTableViewControllerDelegate<NSObject>

// Tells the delegate to refresh the session view.
- (void)refreshSessionsViewRecentTabsTableViewController:
    (LegacyRecentTabsTableViewController*)controller;

@end
