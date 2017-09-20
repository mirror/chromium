// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_SNAPSHOTS_SNAPSHOT_PROVIDER_H_
#define IOS_CHROME_BROWSER_SNAPSHOTS_SNAPSHOT_PROVIDER_H_

#import <UIKit/UIKit.h>

// Type for block that provides a snapshot image.
typedef void (^SnapshotProvidingBlock)(UIImage*);

// Protocol for providing a snapshot.
@protocol SnapshotProvider
// Returns snapshot in block.
- (void)provideSnapshot:(SnapshotProvidingBlock)snapshotProvidingBlock;
@end

#endif  // IOS_CHROME_BROWSER_SNAPSHOTS_SNAPSHOT_PROVIDER_H_
