// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/views/controls/webview/unhandled_keyboard_event_handler.h"

#import "base/mac/foundation_util.h"
#import "ui/views/cocoa/native_widget_mac_nswindow.h"
#include "ui/views/view.h"
#include "ui/views/widget/widget.h"

namespace views {

// static
void UnhandledKeyboardEventHandler::HandleNativeKeyboardEvent(
    gfx::NativeEvent event,
    FocusManager* focus_manager) {
  NSWindow* window = [event window];
  if (!window) {
    View* focused_view =
        focus_manager ? focus_manager->GetFocusedView() : nullptr;
    if (focused_view) {
      window = [focused_view->GetWidget()->GetNativeView() window];
    }
  }
  [base::mac::ObjCCastStrict<NativeWidgetMacNSWindow>(window)
      redispatchKeyEvent:event];
}

}  // namespace views
