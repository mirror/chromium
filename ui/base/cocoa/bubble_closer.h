// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_BASE_COCOA_BUBBLE_CLOSER_H_
#define UI_BASE_COCOA_BUBBLE_CLOSER_H_

#include <objc/objc.h>

#include "base/callback.h"
#include "base/macros.h"
#include "ui/base/ui_base_export.h"
#include "ui/gfx/native_widget_types.h"

#if defined(__OBJC__)
@class BubbleCloserHandleToClosure;
#else
class BubbleCloserHandleToClosure;
#endif

namespace ui {

// Monitors mouse events to allow a regular window to have menu-like popup
// behavior when clicking outside the window. This is needed because macOS
// suppresses window activation events when clicking rapidly.
class UI_BASE_EXPORT BubbleCloser {
 public:
  // Installs an event monitor watching for mouse clicks outside of |window| or
  // any of its child windows. Invokes |on_click_outside| on each event.
  BubbleCloser(gfx::NativeWindow window,
               base::RepeatingClosure on_click_outside);
  ~BubbleCloser();

 private:
  BubbleCloserHandleToClosure* handle_to_closure_;  // Strong.
  id event_tap_;  // Weak. Owned by AppKit.

  DISALLOW_COPY_AND_ASSIGN(BubbleCloser);
};

}  // namespace ui

#endif  // UI_BASE_COCOA_BUBBLE_CLOSER_H_
