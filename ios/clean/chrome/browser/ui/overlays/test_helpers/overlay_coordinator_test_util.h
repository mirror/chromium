// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CLEAN_CHROME_BROWSER_UI_OVERLAYS_TEST_HELPERS_OVERLAY_COORDINATOR_TEST_UTIL_H_
#define IOS_CLEAN_CHROME_BROWSER_UI_OVERLAYS_TEST_HELPERS_OVERLAY_COORDINATOR_TEST_UTIL_H_

@class OverlayCoordinator;

namespace testing {

// Waits for |overlay|'s UI to finish being presented.
void WaitForOverlayPresentation(OverlayCoordinator* overlay);

}  // namespace testing

#endif  // IOS_CLEAN_CHROME_BROWSER_UI_OVERLAYS_TEST_HELPERS_OVERLAY_COORDINATOR_TEST_UTIL_H_
