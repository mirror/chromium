// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/test/earl_grey/chrome_actions.h"

#import "base/mac/foundation_util.h"
#import "ios/chrome/browser/ui/collection_view/cells/collection_view_switch_item.h"
#import "ios/chrome/test/app/chrome_test_util.h"
#import "ios/web/public/test/earl_grey/web_view_actions.h"

#import <EarlGrey/GREYSyntheticEvents.h>

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace chrome_test_util {

id<GREYAction> LongPressElementForContextMenu(const std::string& element_id,
                                              bool triggers_context_menu) {
  return WebViewLongPressElementForContextMenu(
      chrome_test_util::GetCurrentWebState(), element_id,
      triggers_context_menu);
}

id<GREYAction> TurnCollectionViewSwitchOn(BOOL on) {
  id<GREYMatcher> constraints = grey_not(grey_systemAlertViewShown());
  NSString* actionName =
      [NSString stringWithFormat:@"Turn collection view switch to %@ state",
                                 on ? @"ON" : @"OFF"];
  return [GREYActionBlock
      actionWithName:actionName
         constraints:constraints
        performBlock:^BOOL(id collectionViewCell,
                           __strong NSError** errorOrNil) {
          CollectionViewSwitchCell* switchCell =
              base::mac::ObjCCastStrict<CollectionViewSwitchCell>(
                  collectionViewCell);
          UISwitch* switchView = switchCell.switchView;
          if (switchView.on ^ on) {
            id<GREYAction> longPressAction = [GREYActions
                actionForLongPressWithDuration:kGREYLongPressDefaultDuration];
            return [longPressAction perform:switchView error:errorOrNil];
          }
          return YES;
        }];
}

UIView* FindSubviewOfClass(UIView* view, Class klass) {
  if ([view class] == klass)
    return view;
  for (UIView* subview in view.subviews) {
    UIView* viewFound = FindSubviewOfClass(subview, klass);
    if (viewFound) {
      return viewFound;
    }
  }
  return nil;
}

void DragView(UIView* source, UIView* destination) {
  DCHECK_EQ([source window], [destination window]);
  UIWindow* window = [source window];  //[element window];

  CGPoint p1 = source.center;
  CGPoint p2 = destination.center;

  CGFloat dx = p2.x - p1.x;
  CGFloat dy = p2.y - p1.y;

  NSMutableArray* points = [[NSMutableArray alloc] init];

  CGFloat length = sqrt(dx * dx + dy * dy);
  int iterations = (int)length;

  for (int i = 0; i < 60; i++) {
    [points addObject:[NSValue valueWithCGPoint:p1]];
  }

  for (int i = 0; i < iterations; i++) {
    CGPoint p;
    p.x = p1.x + (dx * i) / length;
    p.y = p1.y + (dy * i) / length;
    [points addObject:[NSValue valueWithCGPoint:p]];
  }

  //  [points addObject:[NSValue valueWithCGPoint:p1]];
  //  [points addObject:[NSValue valueWithCGPoint:p1]];
  //  [points addObject:[NSValue valueWithCGPoint:p2]];

  //  for (int i = 0; i < 100; i++) {
  //    [points addObject:[NSValue valueWithCGPoint:p2]];
  //  }

  @autoreleasepool {
    [GREYSyntheticEvents touchAlongPath:points
                       relativeToWindow:window
                            forDuration:3
                             expendable:YES];
  }
}

id<GREYAction> DragViewAndDropTo(UIView* destination) {
  NSString* actionName = @"Drag!!!";
  return [GREYActionBlock
      actionWithName:actionName
         constraints:nil
        performBlock:^BOOL(id view, __strong NSError** errorOrNil) {
          UIView* source = base::mac::ObjCCastStrict<UIView>(view);
          DragView(source, destination);
          return YES;
        }];
}

}  // namespace chrome_test_util
