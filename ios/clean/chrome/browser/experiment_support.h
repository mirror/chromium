// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CLEAN_CHROME_BROWSER_EXPERIMENT_SUPPORT_H_
#define IOS_CLEAN_CHROME_BROWSER_EXPERIMENT_SUPPORT_H_

namespace experiment_support {

// Returns whether the experimental setting for the tap strip has been set.
bool IsTapStripEnabled();

// Returns whether the experimental setting for bottom toolbar has been set.
bool IsBottomToolbarEnabled();

}  // namespace experiment_support

#endif  // IOS_CLEAN_CHROME_BROWSER_EXPERIMENT_SUPPORT_H_
