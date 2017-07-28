// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// ======                        New Architecture                         =====
// =         This code is only used in the new iOS Chrome architecture.       =
// ============================================================================

#ifndef IOS_INTERNAL_CHROME_APP_STEPS_STEP_FEATURES_H_
#define IOS_INTERNAL_CHROME_APP_STEPS_STEP_FEATURES_H_

#import <Foundation/Foundation.h>

namespace step_features {

extern NSString* const kProviders;
extern NSString* const kBundleAndDefaults;
extern NSString* const kChromeMainStarted;
extern NSString* const kChromeMainStopped;
extern NSString* const kForeground;
extern NSString* const kBrowserState;
extern NSString* const kBrowserStateInitialized;
extern NSString* const kMainWindow;
  extern NSString* const kRootCoordinatorStarted;

}  // namespace step_features

#endif  // IOS_INTERNAL_CHROME_APP_STEPS_STEP_FEATURES_H_
