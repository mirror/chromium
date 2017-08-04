// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/fullscreen_placeholder_view.h"

@implementation FullscreenPlaceholderView : NSView

@synthesize textView = textView_;
@synthesize textSize = textSize_;

- (id)initWithFrame:(NSRect)frameRect image:(CGImageRef)screenshot {
  if (self = [super initWithFrame:frameRect]) {
    NSImageView* screenshotView =
        [[[NSImageView alloc] initWithFrame:self.bounds] autorelease];
    CIContext* context = [CIContext contextWithCGContext:nil options:nil];
    CIImage* inputImage =
        [[[CIImage alloc] initWithCGImage:screenshot] autorelease];
    CIFilter* blurFilter = [CIFilter filterWithName:@"CIGaussianBlur"];
    [blurFilter setValue:inputImage forKey:kCIInputImageKey];
    [blurFilter setValue:@15.0 forKey:@"inputRadius"];
    CIImage* result = [blurFilter valueForKey:kCIOutputImageKey];
    CGImageRef cgImage = (CGImageRef)[(
        id)[context createCGImage:result
                         fromRect:[inputImage extent]] autorelease];
    NSImage* screenshotImage =
        [[[NSImage alloc] initWithCGImage:cgImage size:CGSizeZero] autorelease];
    [screenshotView setImage:screenshotImage];

    NSAttributedString* formattedText = [self getStringForCurrentView];
    NSRect positionRect = [self getTextPositionInCurrentView];
    textView_ = [[[NSTextField alloc] initWithFrame:positionRect] autorelease];
    textView_.attributedStringValue = formattedText;
    [textView_.cell setWraps:NO];
    [textView_.cell setScrollable:YES];
    textView_.bezeled = NO;
    textView_.editable = NO;
    textView_.drawsBackground = NO;
    [textView_ setWantsLayer:YES];

    CGColorRef color = CGColorCreateGenericGray(0.3, 0.5);
    [textView_.layer setBackgroundColor:color];
    CGColorRelease(color);
    [textView_.layer setCornerRadius:12];
    [screenshotView addSubview:textView_];

    textView_.autoresizingMask = NSViewMinYMargin | NSViewMaxXMargin |
                                 NSViewMaxYMargin | NSViewMinXMargin |
                                 NSViewWidthSizable | NSViewHeightSizable;
    [screenshotView setImageScaling:NSImageScaleAxesIndependently];
    screenshotView.autoresizingMask = NSViewMinYMargin | NSViewMaxXMargin |
                                      NSViewWidthSizable | NSViewHeightSizable;
    [self addSubview:screenshotView];
  }
  return self;
}

- (void)mouseDown:(NSEvent*)event {
  // This function handles click events on FullscreenPlaceholderView and will be
  // used to exit fullscreen
}

- (void)resizeSubviewsWithOldSize:(CGSize)oldSize {
  [super resizeSubviewsWithOldSize:oldSize];
  textView_.attributedStringValue = [self getStringForCurrentView];
  textView_.frame = [self getTextPositionInCurrentView];
}

- (NSRect)getTextPositionInCurrentView {
  return NSMakeRect(self.frame.size.width / 2 - textSize_.width / 2,
                    self.frame.size.height / 2 - textSize_.height / 2,
                    textSize_.width, textSize_.height);
}

- (NSAttributedString*)getStringForCurrentView {
  NSString* text = @" Click to exit fullscreen ";
  NSDictionary* textAttributes = [[[NSDictionary alloc] initWithDictionary:@{
    NSFontAttributeName :
        [NSFont systemFontOfSize:0.023 * (self.frame.size.width)],
    NSForegroundColorAttributeName :
        [[NSColor whiteColor] colorWithAlphaComponent:0.6]
  }] autorelease];
  textSize_ = [text sizeWithAttributes:textAttributes];
  NSAttributedString* formattedText = [[[NSMutableAttributedString alloc]
      initWithString:text
          attributes:textAttributes] autorelease];
  return formattedText;
}

@end
