// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/metrics/drag_and_drop_recorder.h"

#include "base/metrics/histogram_macros.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

// Reported content of drag events.
enum DragContentForReporting {
  UNKNOWN = 0,
  IMAGE = 1,
  URL = 2,
  TEXT = 3,
  COUNT
};

// Records the DragAndDrop.DragContent histogram for a given |dropSession|.
void RecordDragTypesForSession(id<UIDropSession> dropSession)
    API_AVAILABLE(ios(11.0)) {
  NSDictionary* classToSampleNameMap = @{
        [UIImage class] : @(DragContentForReporting::IMAGE),
        [NSURL class] : @(DragContentForReporting::URL),
        [NSString class] : @(DragContentForReporting::TEXT)
  };
  bool containsAKnownClass = false;
  for (Class klass in classToSampleNameMap) {
    if ([dropSession canLoadObjectsOfClass:klass]) {
      UMA_HISTOGRAM_ENUMERATION("DragAndDrop.DragContent",
                                static_cast<DragContentForReporting>(
                                    [classToSampleNameMap[klass] intValue]),
                                DragContentForReporting::COUNT);
      containsAKnownClass = true;
    }
  }
  if (!containsAKnownClass) {
    UMA_HISTOGRAM_ENUMERATION("DragAndDrop.DragContent",
                              DragContentForReporting::UNKNOWN,
                              DragContentForReporting::COUNT);
  }
}

@interface DragAndDropRecorder ()<UIDropInteractionDelegate> {
  // The currently active drop sessions.
  NSHashTable* dropSessions_;
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
  if (![dropSessions_ containsObject:session]) {
    [dropSessions_ addObject:session];
    RecordDragTypesForSession(session);
  }
  return NO;
}

@end
