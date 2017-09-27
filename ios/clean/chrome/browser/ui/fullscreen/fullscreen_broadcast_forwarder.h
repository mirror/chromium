// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CLEAN_CHROME_BROWSER_UI_FULLSCREEN_FULLSCREEN_BROADCAST_FORWARDER_H_
#define IOS_CLEAN_CHROME_BROWSER_UI_FULLSCREEN_FULLSCREEN_BROADCAST_FORWARDER_H_

#import <Foundation/Foundation.h>

@class ChromeBroadcaster;
class FullscreenModel;

// Object that forwards broadcasted scrolling events to the FullscreenModel.
@interface FullscreenBroadcastForwarder : NSObject

// Initializer for fowarder for |broadcaster| that supplies scrolling events to
// |model|.
- (nullable instancetype)initWithBroadcaster:
                             (nonnull ChromeBroadcaster*)broadcaster
                                       model:(nonnull FullscreenModel*)model
    NS_DESIGNATED_INITIALIZER;
- (nullable instancetype)init NS_UNAVAILABLE;

// Stops observing broadcasted values.
- (void)disconnect;

@end

#endif  // IOS_CLEAN_CHROME_BROWSER_UI_FULLSCREEN_FULLSCREEN_BROADCAST_FORWARDER_H_
