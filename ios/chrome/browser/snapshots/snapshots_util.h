// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_SNAPSHOTS_SNAPSHOTS_UTIL_H_
#define IOS_CHROME_BROWSER_SNAPSHOTS_SNAPSHOTS_UTIL_H_

#import <UIKit/UIKit.h>
#include <vector>

#include "base/callback_forward.h"
#include "base/files/file_path.h"

namespace gfx {
class Image;
}

// Callback used to handle snapshots. The parameter is the snapshot image.
typedef base::Callback<void(const gfx::Image&)> SnapshotViewCallback;

// Clears the application snapshots taken by iOS.
void ClearIOSSnapshots();

// Adds to |snapshotsPaths| all the possible paths to the application's
// snapshots taken by iOS.
void GetSnapshotsPaths(std::vector<base::FilePath>* snapshotsPaths);

// Takes a snapshot of |view| with |target_size|. |callback| is asynchronously
// invoked after performing the snapshot.
void TakeSnapshotOfView(UIView* view,
                        const SnapshotViewCallback& callback,
                        const CGSize target_size);

#endif  // IOS_CHROME_BROWSER_SNAPSHOTS_SNAPSHOTS_UTIL_H_
