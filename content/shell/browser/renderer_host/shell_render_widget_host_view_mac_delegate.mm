// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "content/shell/browser/renderer_host/shell_render_widget_host_view_mac_delegate.h"

#include "base/command_line.h"
#include "content/shell/common/layout_test/layout_test_switches.h"

@interface ShellRenderWidgetHostViewMacDelegate ()
@end

@implementation ShellRenderWidgetHostViewMacDelegate
- (BOOL)handleEvent:(NSEvent*)event {
  // Throw out all native input events if we are running with layout test
  // enabled.
  static bool run_layout_tests =
      base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kRunLayoutTest);
  return run_layout_tests ? YES : NO;
}

- (void)beginGestureWithEvent:(NSEvent*)event {
}

- (void)endGestureWithEvent:(NSEvent*)event {
}

- (void)touchesMovedWithEvent:(NSEvent*)event {
}

- (void)touchesBeganWithEvent:(NSEvent*)event {
}

- (void)touchesCancelledWithEvent:(NSEvent*)event {
}

- (void)touchesEndedWithEvent:(NSEvent*)event {
}

- (void)rendererHandledWheelEvent:(const blink::WebMouseWheelEvent&)event
                         consumed:(BOOL)consumed {
}

- (void)rendererHandledGestureScrollEvent:(const blink::WebGestureEvent&)event
                                 consumed:(BOOL)consumed {
}

@end
