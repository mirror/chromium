// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_OMNIBOX_AUTOCOMPLETE_RESULT_WRAPPER_H_
#define IOS_CHROME_BROWSER_UI_OMNIBOX_AUTOCOMPLETE_RESULT_WRAPPER_H_

#import <UIKit/UIKit.h>

class GURL;

@protocol AutocompleteResultWrapper<NSObject>
- (BOOL)supportsDeletion;
- (BOOL)hasAnswer;
- (BOOL)hasImage;
- (BOOL)isURL;
- (NSAttributedString*)detailText;
- (NSInteger)numberOfLines;
- (NSAttributedString*)text;
- (BOOL)isAppendable;
- (GURL)imageURL;
- (int)imageId;
@end

#endif  // IOS_CHROME_BROWSER_UI_OMNIBOX_AUTOCOMPLETE_RESULT_WRAPPER_H_
