// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/fullscreen_placeholder_view.h"
#include "base/mac/sdk_forward_declarations.h"

@implementation FullscreenPlaceholderView {
  NSTextField* textView_;
}

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

    textView_ = [NSTextField labelWithString:@" Click to exit fullscreen "];
    [textView_ setTextColor:[[NSColor whiteColor] colorWithAlphaComponent:0.6]];
    [textView_
        setFont:[NSFont systemFontOfSize:0.023 * (self.frame.size.width)]];
    [textView_ sizeToFit];
    [textView_
        setFrameOrigin:NSMakePoint(
                           NSMidX(self.bounds) - NSMidX(textView_.bounds),
                           NSMidY(self.bounds) - NSMidY(textView_.bounds))];

    [textView_.cell setWraps:NO];
    [textView_.cell setScrollable:YES];
    [textView_ setBezeled:NO];
    [textView_ setEditable:NO];
    [textView_ setDrawsBackground:NO];
    [textView_ setWantsLayer:YES];

    NSColor* color = [NSColor colorWithCalibratedWhite:0.3 alpha:0.5];
    [textView_.layer setBackgroundColor:color.CGColor];
    [textView_.layer setCornerRadius:12];
    [screenshotView addSubview:textView_];

    [screenshotView setImageScaling:NSImageScaleAxesIndependently];
    [screenshotView setAutoresizingMask:NSViewMinYMargin | NSViewMaxXMargin |
                                        NSViewWidthSizable |
                                        NSViewHeightSizable];
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
  [textView_ setFont:[NSFont systemFontOfSize:0.023 * (self.frame.size.width)]];
  [textView_ sizeToFit];
  [textView_
      setFrameOrigin:NSMakePoint(
                         NSMidX(self.bounds) - NSMidX(textView_.bounds),
                         NSMidY(self.bounds) - NSMidY(textView_.bounds))];
}

@end
