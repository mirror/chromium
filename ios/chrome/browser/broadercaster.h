// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_BROADERCASTER_H_
#define IOS_CHROME_BROWSER_BROADERCASTER_H_

#import <UIKit/UIKit.h>

// Broadcaster is used to loosely couple objects across the application.
// The objects can broadcast themselves through broadcaster, and other objects
// can observe them.
// The name is chosen to avoid possible confusion with ChromeBroadcaster.
@interface Broadercaster : NSObject

// Default instance.
+ (Broadercaster*)defaultBroadercasterer;

// The notification to observe for class.
+ (NSString*)notificationForClass:(Class)klass;

// Method to add object as the broadcasted one.
- (void)broadcast:(id)object;

// The method to add observers.
- (void)observe:(Class)klass selector:(SEL)selector observer:(id)observer;

// Returns the broadcasted object;
- (id)objectForClass:(Class)klass;

@end

// convenience macro for use of @broadcastify(xxx);
#define broadcastify(var) \
  autoreleasepool { [[Broadercaster defaultBroadercasterer] broadcast:var]; }

// convenience macro for use of @observe(BrowserViewController, bvcChanged)
#define observe(klass, selector)                             \
  autoreleasepool {                                          \
    [[Broadercaster defaultBroadercasterer] observe:klass    \
                                           selector:selector \
                                           observer:self];   \
  }

#endif  // IOS_CHROME_BROWSER_BROADERCASTER_H_
