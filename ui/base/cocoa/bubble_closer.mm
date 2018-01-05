// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/base/cocoa/bubble_closer.h"

#import <AppKit/AppKit.h>

// A ref-counted object to allow the Closer to communicate with the block.
// AppKit posts to all event monitors active when an event is received, so it's
// possible for one monitor to remove another, and the removed monitor to still
// see the event.
@interface BubbleCloserHandleToClosure : NSObject {
 @public
  base::RepeatingClosure onClickOutside;
}
@end
@implementation BubbleCloserHandleToClosure
@end

namespace ui {

BubbleCloser::BubbleCloser(gfx::NativeWindow window,
                           base::RepeatingClosure on_click_outside) {
  handle_to_closure_ = [[BubbleCloserHandleToClosure alloc] init];
  handle_to_closure_->onClickOutside = on_click_outside;

  // Note that |window| will be retained when captured by the block below.
  // |this| is captured, but not retained.
  auto block = ^NSEvent*(NSEvent* event) {
    NSWindow* event_window = [event window];
    if ([event_window isSheet])
      return event;

    // Do not close the bubble if the event happened on a window with a
    // higher level.  For example, the content of a browser action bubble
    // opens a calendar picker window with NSPopUpMenuWindowLevel, and a
    // date selection closes the picker window, but it should not close
    // the bubble.
    if ([event_window level] > [window level])
      return event;

    // If the event is in |window|'s hierarchy, do not close the bubble.
    NSWindow* ancestor = event_window;
    while (ancestor) {
      if (ancestor == window)
        return event;
      ancestor = [ancestor parentWindow];
    }

    if (handle_to_closure_->onClickOutside)
      handle_to_closure_->onClickOutside.Run();  // Note: May delete |this|.
    return event;
  };
  event_tap_ =
      [NSEvent addLocalMonitorForEventsMatchingMask:NSLeftMouseDownMask |
                                                    NSRightMouseDownMask
                                            handler:block];
}

BubbleCloser::~BubbleCloser() {
  handle_to_closure_->onClickOutside.Reset();
  [NSEvent removeMonitor:event_tap_];
  [handle_to_closure_ release];  // Note the block may still have a reference.
}

}  // namespace ui
