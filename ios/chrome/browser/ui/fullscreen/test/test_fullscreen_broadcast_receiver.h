// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_FULLSCREEN_TEST_TEST_FULLSCREEN_BROADCAST_RECEIVER_H_
#define IOS_CHROME_BROWSER_UI_FULLSCREEN_TEST_TEST_FULLSCREEN_BROADCAST_RECEIVER_H_

#import "ios/chrome/browser/ui/fullscreen/fullscreen_broadcast_receiver.h"

// A test object conforming to FullscreenBroadcastReceiving where received
// values are stored with no effect.
@interface TestFullscreenBroadcastReceiver
    : NSObject<FullscreenBroadcastReceiving>
@end

#endif  // IOS_CHROME_BROWSER_UI_FULLSCREEN_TEST_TEST_FULLSCREEN_BROADCAST_RECEIVER_H_
