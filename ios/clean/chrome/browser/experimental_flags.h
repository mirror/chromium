// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CLEAN_CHROME_BROWSER_EXPERIMENTAL_FLAGS_H_
#define IOS_CLEAN_CHROME_BROWSER_EXPERIMENTAL_FLAGS_H_

namespace experimental_flags {

// Returns YES if the experimental setting for the tap strip has been set.
bool IsTapStripEnabled();

// Returns YES if the experimental setting for bottom toolbar has been set.
bool IsBottomToolbarEnabled();

}  // namespace experimental_flags

#endif  // IOS_CLEAN_CHROME_BROWSER_EXPERIMENTAL_FLAGS_H_
