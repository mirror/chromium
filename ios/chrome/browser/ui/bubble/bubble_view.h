// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_BUBBLE_BUBBLE_VIEW_H_
#define IOS_CHROME_BROWSER_UI_BUBBLE_BUBBLE_VIEW_H_

#import <UIKit/UIKit.h>

// Direction for the bubble to point.
typedef NS_ENUM(NSInteger, BubbleArrowDirection) {
  // Bubble is below the target UI element and the arrow is pointing up.
  BubbleArrowDirectionUp = 0,
  // Bubble is above the target UI element and the arrow is pointing down.
  BubbleArrowDirectionDown = 1,
};

// Speech bubble shaped view that displays a message.
@interface BubbleView : UIView

// Initializes with the given text and direction that the bubble should point.
- (instancetype)initWithText:(NSString*)text
              arrowDirection:(BubbleArrowDirection)direction
    NS_DESIGNATED_INITIALIZER;

- (instancetype)initWithFrame:(CGRect)frame NS_UNAVAILABLE;

- (instancetype)initWithCoder:(NSCoder*)aDecoder NS_UNAVAILABLE;

- (instancetype)init NS_UNAVAILABLE;

// Position the tip of the arrow on |anchorPoint|.
- (void)anchorOnPoint:(CGPoint)anchorPoint
       arrowDirection:(BubbleArrowDirection)direction;

@end

#endif  // IOS_CHROME_BROWSER_UI_BUBBLE_BUBBLE_VIEW_H_
