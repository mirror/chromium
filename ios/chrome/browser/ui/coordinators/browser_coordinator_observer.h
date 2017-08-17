// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_COORDINATORS_BROWSER_COORDINATOR_OBSERVER_H_
#define IOS_CHROME_BROWSER_UI_COORDINATORS_BROWSER_COORDINATOR_OBSERVER_H_

#import <Foundation/Foundation.h>

@class BrowserCoordinator;

// An observer protocol for BrowserCoordinator events.
@protocol BrowserCoordinatorObserver<NSObject>

@optional

// Tells the observer that |coordinator| was started.
- (void)browserCoordinatorDidStart:(BrowserCoordinator*)coordinator;

// Tells the observer that |coordinator|'s consumer has started.  This will be
// called after consumer presentation animation code has finished.
- (void)browserCoordinatorConsumerDidStart:(BrowserCoordinator*)coordinator;

// Tells the observer that |coordinator| was stopped.
- (void)browserCoordinatorDidStop:(BrowserCoordinator*)coordinator;

// Tells the observer that |coordinator|'s consumer has stopped.  This will be
// called after consumer dismissal animation code has finished.
- (void)browserCoordinatorConsumerDidStop:(BrowserCoordinator*)coordinator;

// Tells the observer that |coordinator| is being destroyed.
- (void)browserCoordinatorWasDestroyed:(BrowserCoordinator*)coordinator;

@end

#endif  // IOS_CHROME_BROWSER_UI_COORDINATORS_BROWSER_COORDINATOR_OBSERVER_H_
