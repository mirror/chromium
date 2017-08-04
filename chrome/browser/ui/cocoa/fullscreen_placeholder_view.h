// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_FULLSCREEN_PLACEHOLDER_VIEW_H_
#define CHROME_BROWSER_UI_COCOA_FULLSCREEN_PLACEHOLDER_VIEW_H_

#import <Cocoa/Cocoa.h>
#import <QuartzCore/CoreImage.h>

@class FullscreenPlaceholderView;

@interface FullscreenPlaceholderView : NSView

// Properties used to update the position and font size of the text on resize of
// the view
@property(nonatomic, assign) NSTextField* textView;
@property(nonatomic, assign) CGSize textSize;

// Formats the screenshot displayed on the tab content area when on fullscreen
- (id)initWithFrame:(NSRect)frameRect image:(CGImageRef)screenshot;

// Handles click events on the view to exit fullscreen
- (void)mouseDown:(NSEvent*)event;

// Returns the updated position rectangle to center textView in the view
- (NSRect)getTextPositionInCurrentView;

// Returns the string with the updated font size for the view's width
- (NSAttributedString*)getStringForCurrentView;

@end  // @interface FullscreenPlaceholderView : NSView

#endif  // CHROME_BROWSER_UI_COCOA_FULLSCREEN_PLACEHOLDER_VIEW_H_
