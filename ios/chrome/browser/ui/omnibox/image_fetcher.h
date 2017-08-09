// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_OMNIBOX_IMAGE_FETCHER_H_
#define IOS_CHROME_BROWSER_UI_OMNIBOX_IMAGE_FETCHER_H_

@protocol ImageFetcher<NSObject>
- (void)fetchImage:(GURL)imageURL completion:(void (^)(UIImage*))completion;
@end

#endif  // IOS_CHROME_BROWSER_UI_OMNIBOX_IMAGE_FETCHER_H_
