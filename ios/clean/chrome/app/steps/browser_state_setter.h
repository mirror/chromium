// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// ======                        New Architecture                         =====
// =         This code is only used in the new iOS Chrome architecture.       =
// ============================================================================

#ifndef IOS_INTERNAL_CHROME_APP_STEPS_BROWSER_STATE_SETTER_H_
#define IOS_INTERNAL_CHROME_APP_STEPS_BROWSER_STATE_SETTER_H_

#import <Foundation/Foundation.h>

#import "ios/clean/chrome/app/steps/application_step.h"
#import "ios/clean/chrome/app/steps/simple_application_step.h"

@interface BrowserStateSetter : SimpleApplicationStep<ApplicationStep>
@end

#endif  // IOS_INTERNAL_CHROME_APP_STEPS_BROWSER_STATE_SETTER_H_
