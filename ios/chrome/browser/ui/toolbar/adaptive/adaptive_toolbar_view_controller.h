// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_TOOLBAR_ADAPTIVE_ADAPTIVE_TOOLBAR_VIEW_CONTROLLER_H_
#define IOS_CHROME_BROWSER_UI_TOOLBAR_ADAPTIVE_ADAPTIVE_TOOLBAR_VIEW_CONTROLLER_H_

#import <UIKit/UIKit.h>

#import "ios/chrome/browser/ui/toolbar/clean/toolbar_consumer.h"
#import "ios/chrome/browser/ui/toolbar/clean/toolbar_type.h"

@protocol ApplicationCommands;
@class ToolbarButtonFactory;
@protocol BrowserCommands;

// ViewController for the adaptive toolbar. This ViewController is the same for
// all toolbar sub-implementation (primary and secondary).
@interface AdaptiveToolbarViewController : UIViewController<ToolbarConsumer>

// Initializes the toolbar with the |buttonFactory| and its |type|, PRIMARY or
// SECONDARY.
- (instancetype)initWithButtonFactory:(ToolbarButtonFactory*)buttonFactory
                                 type:(ToolbarType)type;

- (instancetype)init NS_UNAVAILABLE;
- (instancetype)initWithCoder:(NSCoder*)aDecoder NS_UNAVAILABLE;
- (instancetype)initWithNibName:(NSString*)nibNameOrNil
                         bundle:(NSBundle*)nibBundleOrNil NS_UNAVAILABLE;

// Dispatcher for the ViewController.
@property(nonatomic, weak) id<ApplicationCommands, BrowserCommands> dispatcher;

// Sets the location bar view, containing the omnibox.
- (void)setLocationBarView:(UIView*)locationBarView;

@end

#endif  // IOS_CHROME_BROWSER_UI_TOOLBAR_ADAPTIVE_ADAPTIVE_TOOLBAR_VIEW_CONTROLLER_H_
