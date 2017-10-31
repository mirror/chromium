// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_TOOLBAR_TOOLBAR_OWNER_H_
#define IOS_CHROME_BROWSER_UI_TOOLBAR_TOOLBAR_OWNER_H_

#import <Foundation/Foundation.h>

@class ToolbarController;
@protocol ToolbarSnapshotProviding;

@protocol ToolbarOwner<NSObject>

// Snapshot provider for the toolbar owner by this class.
@property(nonatomic, strong, readonly) id<ToolbarSnapshotProviding>
    toolbarSnapshotProvider;

@optional
// Returns the height of the toolbar owned by the implementing class.
- (CGFloat)toolbarHeight;

@end

#endif  // IOS_CHROME_BROWSER_UI_TOOLBAR_TOOLBAR_OWNER_H_
