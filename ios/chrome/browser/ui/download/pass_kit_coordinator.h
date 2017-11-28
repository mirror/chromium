// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_DOWNLOAD_PASS_KIT_COORDINATOR_H_
#define IOS_CHROME_BROWSER_UI_DOWNLOAD_PASS_KIT_COORDINATOR_H_

#import <PassKit/PassKit.h>

#import "ios/chrome/browser/chrome_coordinator.h"
#import "ios/chrome/browser/download/pass_kit_tab_helper_delegate.h"

namespace web {
class WebState;
}  // namespace web

@interface PassKitCoordinator : ChromeCoordinator<PassKitTabHelperDelegate>
@property(nonatomic) web::WebState* webState;
@property(nonatomic) PKPass* pass;
@end

#endif  // IOS_CHROME_BROWSER_UI_DOWNLOAD_PASS_KIT_COORDINATOR_H_
