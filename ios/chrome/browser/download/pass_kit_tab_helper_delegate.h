// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_DOWNLOAD_PASS_KIT_TAB_HELPER_DELEGATE_H_
#define IOS_CHROME_BROWSER_DOWNLOAD_PASS_KIT_TAB_HELPER_DELEGATE_H_

#import <Foundation/Foundation.h>

class PassKitTabHelper;
@class PKPass;

@protocol PassKitTabHelperDelegate<NSObject>

- (void)passKitTabHelper:(PassKitTabHelper*)tabHelper
    presentDialogForPass:(PKPass*)pass
                   error:(NSError*)error;

@end

#endif  // IOS_CHROME_BROWSER_DOWNLOAD_PASS_KIT_TAB_HELPER_DELEGATE_H_
