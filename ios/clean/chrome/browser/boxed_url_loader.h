// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CLEAN_CHROME_BROWSER_BOXED_URL_LOADER_H_
#define IOS_CLEAN_CHROME_BROWSER_BOXED_URL_LOADER_H_

#import "ios/chrome/browser/ui/url_loader.h"

// URL loader to be used for boxed components.
@interface BoxedURLLoader : NSObject<UrlLoader>

// TODO(crbug.com/740793): Remove alert there is a way to open URL.
@property(nonatomic, strong) UIViewController* viewControllerForAlert;

@end

#endif  // IOS_CLEAN_CHROME_BROWSER_BOXED_URL_LOADER_H_
