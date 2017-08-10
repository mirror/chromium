// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// ======                        New Architecture                         =====
// =         This code is only used in the new iOS Chrome architecture.       =
// ============================================================================

#import "ios/clean/chrome/app/steps/step_features.h"

namespace step_features {

NSString* const kProviders = @"providers";
NSString* const kBundleAndDefaults = @"bundle-and-defualts";
NSString* const kChromeMainStarted = @"chrome-main-started";
NSString* const kChromeMainStopped = @"chrome-main-stopped";
NSString* const kForeground = @"foreground";
NSString* const kBrowserState = @"browser-state";
NSString* const kBrowserStateInitialized = @"browser-state-init";
NSString* const kMainWindow = @"mainWindow";
NSString* const kRootCoordinatorStarted = @"rootCoordinatorStarted";

}  // namespace step_features
