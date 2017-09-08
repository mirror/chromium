// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/test/earl_grey/chrome_actions.h"

#import "base/mac/foundation_util.h"
#import "ios/chrome/browser/ui/collection_view/cells/collection_view_switch_item.h"
#import "ios/chrome/test/app/chrome_test_util.h"
#import "ios/web/public/test/earl_grey/web_view_actions.h"

//#import "ios/third_party/earl_grey/src/EarlGrey/Matcher/GREYMatchers.h"
//#import "ios/third_party/earl_grey/src/EarlGrey/Provider/GREYElementProvider.h"
//#import "ios/third_party/earl_grey/src/EarlGrey/Provider/GREYUIWindowProvider.h"


#import "ios/third_party/earl_grey/src/EarlGrey/Event/GREYSyntheticEvents.h"


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

  UIWindow *window = [source window];//[element window];


  CGPoint p1 = source.center;
  CGPoint p2 = destination.center;

  CGFloat dx = p2.x - p1.x;
  CGFloat dy = p2.y - p1.y;

  NSMutableArray* points = [[NSMutableArray alloc] init];

  CGFloat length = sqrt(dx * dx + dy *dy);
  int iterations = (int)length;
  for (int i = 0; i < iterations; i++) {
    CGPoint p;
    p.x = p1.x + (dx * i) / length;
    p.y = p1.y + (dy * i) / length;
    [points addObject:[NSValue valueWithCGPoint:p]];
  }


  [GREYSyntheticEvents touchAlongPath:points
                     relativeToWindow:window
                          forDuration:10
                           expendable:YES];

}


}  // namespace chrome_test_util









#if 0



//#import "Action/GREYPickerAction.h"
//
//#import "Additions/NSError+GREYAdditions.h"
//#import "Additions/UIView+GREYAdditions.h"
//#import "Common/GREYDefines.h"
//#import "Common/GREYError.h"
//#import "Core/GREYInteraction.h"
//#import "Matcher/GREYAllOf.h"
//#import "Matcher/GREYMatchers.h"
//#import "Matcher/GREYNot.h"
//#import "Synchronization/GREYTimedIdlingResource.h"

@interface GREYDragAndDropAction() {
  id<GREYMatcher> _destinationMatcher;
}
@end

@class GREYMatcher;

@implementation GREYDragAndDropAction
  
  
  
  
  
  - (instancetype)initWithDestination:(id<GREYMatcher>)destinationMatcher {
    NSString *name =
    [NSString stringWithFormat:@"Drag to '%@'", _destinationMatcher];
    self = [super initWithName:name constraints:grey_allOf(grey_interactable(),
                                                           grey_userInteractionEnabled(),
                                                           grey_not(grey_systemAlertViewShown()),
                                                           grey_kindOfClass([UIPickerView class]),
                                                           nil)];
    if (self) {

      _destinationMatcher = destinationMatcher;
          }
    return self;

    return nil;
  }


#pragma mark - GREYAction
  
  - (BOOL)perform:(id)element error:(__strong NSError **)errorOrNil {
    if (![self satisfiesConstraintsForElement:element error:errorOrNil]) {
      return NO;
    }


    UIWindow *window = [element window];


    CGPoint p1;
    CGPoint p2;

    CGFloat dx = p1.x - p2.x;
    CGFloat dy = p1.y - p2.y;

    NSMutableArray* points = [[NSMutableArray alloc] init];

    CGFloat length = sqrt(dx * dx + dy *dy);
    int iterations = (int)length;
    for (int i = 0; i < iterations; i++) {
      CGPoint p;
      p.x = p1.x + (dx * i) / length;
      p.y = p1.y + (dy * i) / length;
      [points addObject:[NSValue valueWithCGPoint:p]];
    }


    [GREYSyntheticEvents touchAlongPath:points
                       relativeToWindow:window
                            forDuration:10
                             expendable:YES];

  };

/*

    GREYElementProvider *entireRootHierarchyProvider =
    [GREYElementProvider providerWithRootProvider:[GREYUIWindowProvider providerWithAllWindows]];

    GREYElementFinder *elementFinder = [[GREYElementFinder alloc] initWithMatcher:_destinationMatcher];

    NSArray *elements = [elementFinder elementsMatchedInProvider:entireRootHierarchyProvider];



    UIView* v1 = element;

    UIView* v2 = [elements firstObject];


    NSLog(@"%@ %@", v1, v2);*/
    return YES;



    //+ (NSArray *)grey_touchPathWithStartPoint:(CGPoint)startPoint
    //endPoint:(CGPoint)endPoint
    //duration:(CFTimeInterval)duration




    /*

     GREYElementProvider *entireRootHierarchyProvider =
     [GREYElementProvider providerWithRootProvider:[GREYUIWindowProvider providerWithAllWindows]];
     GREYElementFinder *elementFinder = [[GREYElementFinder alloc] initWithMatcher:elementMatcher];
     NSArray *elements = [elementFinder elementsMatchedInProvider:entireRootHierarchyProvider];
     
     
     NSInteger componentCount = [pickerView.dataSource numberOfComponentsInPickerView:pickerView];
     
     if (componentCount < _column) {
     NSString *description = [NSString stringWithFormat:@"Invalid column on picker view [P] "
     @"cannot find the column %lu.",
     (unsigned long)_column];
     NSDictionary *glossary = @{ @"P" : [pickerView description] };
     GREYPopulateErrorNotedOrLog(errorOrNil,
     kGREYInteractionErrorDomain,
     kGREYInteractionActionFailedErrorCode,
     description,
     glossary);
     
     return NO;
     }
     
     NSInteger columnRowCount = [pickerView.dataSource pickerView:pickerView
     numberOfRowsInComponent:_column];
     
     SEL titleForRowSelector = @selector(pickerView:titleForRow:forComponent:);
     SEL viewForRowSelector = @selector(pickerView:viewForRow:forComponent:reusingView:);
     
     for (NSInteger rowIndex = 0; rowIndex < columnRowCount; rowIndex++) {
     NSString *rowTitle;
     if ([pickerView.delegate respondsToSelector:titleForRowSelector]) {
     rowTitle = [pickerView.delegate pickerView:pickerView
     titleForRow:rowIndex
     forComponent:_column];
     } else if ([pickerView.delegate respondsToSelector:viewForRowSelector]) {
     UIView *rowView = [pickerView.delegate pickerView:pickerView
     viewForRow:rowIndex
     forComponent:_column
     reusingView:nil];
     if (![rowView isKindOfClass:[UILabel class]]) {
     NSArray *labels = [rowView grey_childrenAssignableFromClass:[UILabel class]];
     UILabel *label = (labels.count > 0 ? labels[0] : nil);
     rowTitle = label.text;
     } else {
     rowTitle = [((UILabel *)rowView) text];
     }
     }
     if ([rowTitle isEqualToString:_value]) {
     [pickerView selectRow:rowIndex inComponent:_column animated:YES];
     if ([pickerView.delegate
     respondsToSelector:@selector(pickerView:didSelectRow:inComponent:)]) {
     [pickerView.delegate pickerView:pickerView didSelectRow:rowIndex inComponent:_column];
     }
     // UIPickerView does a delayed animation. We don't track delayed animations, therefore
     // we have to track it manually
     [GREYTimedIdlingResource resourceForObject:pickerView
     thatIsBusyForDuration:0.5
     name:@"UIPickerView"];
     return YES;
     #endif     }
     }
     GREYPopulateErrorOrLog(errorOrNil,
     kGREYInteractionErrorDomain,
     kGREYInteractionActionFailedErrorCode,
     @"UIPickerView does not contain desired value!");
     
     */
//    return NO;
  }
  @end



#endif
