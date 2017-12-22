// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_IMAGE_UTIL_IMAGE_SAVER_H_
#define IOS_CHROME_BROWSER_UI_IMAGE_UTIL_IMAGE_SAVER_H_

#import <UIKit/UIKit.h>

#include "components/image_fetcher/core/request_metadata.h"

@interface ImageSaver : NSObject

- (instancetype)initWithBaseViewController:
    (UIViewController*)baseViewController;

- (void)saveImageData:(NSData*)data
         withMetadata:(const image_fetcher::RequestMetadata&)metadata;

@end

#endif  // IOS_CHROME_BROWSER_UI_IMAGE_UTIL_IMAGE_SAVER_H_
