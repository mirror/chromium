// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/metrics/drag_and_drop_recorder.h"

#include "base/metrics/histogram_macros.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

// Records a range of boolean histogram for a given |dropSession|.
void RecordDropTypesForSession(id<UIDropSession> dropSession)
    API_AVAILABLE(ios(11.0)) {
  bool containsURL = [dropSession canLoadObjectsOfClass:[NSURL class]];
  bool containsText = [dropSession canLoadObjectsOfClass:[NSString class]];
  bool containsImage = [dropSession canLoadObjectsOfClass:[UIImage class]];
  bool containsExclusivelySomethingElse =
      !containsURL && !containsText && !containsImage;
  UMA_HISTOGRAM_BOOLEAN("DragAndDrop.DropContainsURL", containsURL);
  UMA_HISTOGRAM_BOOLEAN("DragAndDrop.DropContainsText", containsText);
  UMA_HISTOGRAM_BOOLEAN("DragAndDrop.DropContainsImage", containsImage);
  UMA_HISTOGRAM_BOOLEAN("DragAndDrop.DropContainsSomethingElse",
                        containsExclusivelySomethingElse);
}

@interface DragAndDropRecorder ()<UIDropInteractionDelegate> {
  // The currently active drop sessions.
  NSHashTable* dropSessions_;
  __weak id lastDropSession_;
}
@end

@implementation DragAndDropRecorder

- (instancetype)initWithView:(UIView*)view {
  self = [super init];
  if (self) {
    if (@available(iOS 11, *)) {
      dropSessions_ = [NSHashTable weakObjectsHashTable];
      UIDropInteraction* dropInteraction =
          [[UIDropInteraction alloc] initWithDelegate:self];
      [view addInteraction:dropInteraction];
    }
  }
  return self;
}

- (BOOL)dropInteraction:(UIDropInteraction*)interaction
       canHandleSession:(id<UIDropSession>)session API_AVAILABLE(ios(11.0)) {
  // |-dropInteraction:canHandleSession:| can be called multiple times for the
  // same drop session.
  // Maintain a set of weak references to these sessions to make sure metrics
  // are recorded only once per drop session.
  lastDropSession_ = session;

  if (![dropSessions_ containsObject:session]) {
    lastDropSession_ = session;
    [dropSessions_ addObject:session];
    RecordDropTypesForSession(session);
  }
  return NO;
}

@end
