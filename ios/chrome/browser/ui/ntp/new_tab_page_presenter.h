// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_NTP_NEW_TAB_PAGE_PRESENTER_H_
#define IOS_CHROME_BROWSER_UI_NTP_NEW_TAB_PAGE_PRESENTER_H_

#import <Foundation/Foundation.h>

namespace web {
class WebState;
}

// Protocol Bar

@protocol NewTabPagePresenter<NSObject>

- (void)showNTP:(web::WebState*)webState;
- (void)hideNTP:(web::WebState*)webState;

@end

#endif  // IOS_CHROME_BROWSER_UI_NTP_NEW_TAB_PAGE_PRESENTER_H_
