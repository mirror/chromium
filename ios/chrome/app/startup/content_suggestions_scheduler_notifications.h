// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_APP_STARTUP_CONTENT_SUGGESTIONS_SCHEDULER_NOTIFICATIONS_H_
#define IOS_CHROME_APP_STARTUP_CONTENT_SUGGESTIONS_SCHEDULER_NOTIFICATIONS_H_

#import <Foundation/Foundation.h>

namespace ios {
class ChromeBrowserState;
}

@interface ContentSuggestionsSchedulerNotifications : NSObject

+ (void)notifyColdStart:(ios::ChromeBrowserState*)browserState;

+ (void)notifyForeground:(ios::ChromeBrowserState*)browserState;

@end

#endif  // IOS_CHROME_APP_STARTUP_CONTENT_SUGGESTIONS_SCHEDULER_NOTIFICATIONS_H_
