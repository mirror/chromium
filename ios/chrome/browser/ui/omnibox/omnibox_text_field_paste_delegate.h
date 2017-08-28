// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_OMNIBOX_OMNIBOX_TEXT_FIELD_PASTE_DELEGATE_H_
#define IOS_CHROME_BROWSER_UI_OMNIBOX_OMNIBOX_TEXT_FIELD_PASTE_DELEGATE_H_

#import <UIKit/UIKit.h>

// Implements UITextPasteDelegate to workaround http://crbug.com/755620.
@interface OmniboxTextFieldPasteDelegate : NSObject
@end

// Since UITextPasteDelegate is only defined on iOS 11, compiler wants these
// guards.
#ifdef __IPHONE_11_0
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wpartial-availability"

@interface OmniboxTextFieldPasteDelegate (
    UITextPasteDelegate)<UITextPasteDelegate>
@end

#pragma clang diagnostic pop
#endif  // __IPHONE_11_0

#endif  // IOS_CHROME_BROWSER_UI_OMNIBOX_OMNIBOX_TEXT_FIELD_PASTE_DELEGATE_H_
