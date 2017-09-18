// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/chrome/browser/snapshots/snapshots_util.h"

#import <UIKit/UIKit.h>

#include "base/bind.h"
#include "base/files/file_util.h"
#include "base/ios/ios_util.h"
#include "base/location.h"
#include "base/mac/foundation_util.h"
#include "base/macros.h"
#include "base/path_service.h"
#include "base/strings/stringprintf.h"
#include "base/task_scheduler/post_task.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "ui/gfx/image/image.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {
const char* kOrientationDescriptions[] = {
    "LandscapeLeft",
    "LandscapeRight",
    "Portrait",
    "PortraitUpsideDown",
};
}  // namespace

void ClearIOSSnapshots() {
  // Generates a list containing all the possible snapshot paths because the
  // list of snapshots stored on the device can't be obtained programmatically.
  std::vector<base::FilePath> snapshotsPaths;
  GetSnapshotsPaths(&snapshotsPaths);
  for (base::FilePath snapshotPath : snapshotsPaths) {
    base::PostTaskWithTraits(
        FROM_HERE, {base::MayBlock(), base::TaskPriority::BACKGROUND},
        base::BindOnce(base::IgnoreResult(&base::DeleteFile), snapshotPath,
                       false));
  }
}

void GetSnapshotsPaths(std::vector<base::FilePath>* snapshotsPaths) {
  DCHECK(snapshotsPaths);
  base::FilePath snapshotsDir;
  PathService::Get(base::DIR_CACHE, &snapshotsDir);
  // Snapshots are located in a path with the bundle ID used twice.
  snapshotsDir = snapshotsDir.Append("Snapshots")
                     .Append(base::mac::BaseBundleID())
                     .Append(base::mac::BaseBundleID());
  const char* retinaSuffix = "";
  CGFloat scale = [UIScreen mainScreen].scale;
  if (scale == 2) {
    retinaSuffix = "@2x";
  } else if (scale == 3) {
    retinaSuffix = "@3x";
  }
  for (unsigned int i = 0; i < arraysize(kOrientationDescriptions); i++) {
    std::string snapshotFilename =
        base::StringPrintf("UIApplicationAutomaticSnapshotDefault-%s%s.png",
                           kOrientationDescriptions[i], retinaSuffix);
    base::FilePath snapshotPath = snapshotsDir.Append(snapshotFilename);
    snapshotsPaths->push_back(snapshotPath);
  }
}

void TakeSnapshotOfView(UIView* view,
                        const SnapshotViewCallback& callback,
                        const CGSize target_size) {
  DCHECK(view);
  UIImage* snapshot = nil;
  if (view && !CGRectIsEmpty(view.bounds)) {
    CGFloat scaled_height =
        view.bounds.size.height * target_size.width / view.bounds.size.width;
    CGRect scaled_rect = CGRectMake(0, 0, target_size.width, scaled_height);
    UIGraphicsBeginImageContextWithOptions(target_size, YES, 0);
    [view drawViewHierarchyInRect:scaled_rect afterScreenUpdates:NO];
    snapshot = UIGraphicsGetImageFromCurrentImageContext();
    UIGraphicsEndImageContext();
  }
  base::SequencedTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::Bind(callback, gfx::Image(snapshot)));
}
