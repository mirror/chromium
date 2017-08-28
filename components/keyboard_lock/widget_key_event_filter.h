// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_KEYBOARD_LOCK_WIDGET_KEY_EVENT_FILTER_H_
#define COMPONENTS_KEYBOARD_LOCK_WIDGET_KEY_EVENT_FILTER_H_

#include "components/keyboard_lock/key_event_filter.h"

namespace content {
class WebContents;
class RenderWidgetHost;
}  // namespace

namespace keyboard_lock {

// An implementation of KeyEventFilter to forward key events to the
// RenderWidgetHost.
class WidgetKeyEventFilter : public KeyEventFilter {
 public:
  explicit WidgetKeyEventFilter(const content::WebContents* contents);
  ~WidgetKeyEventFilter() override;

 private:
  // KeyEventFilter implementations
  bool OnKeyDown(ui::KeyboardCode code, int flags) override;
  bool OnKeyUp(ui::KeyboardCode code, int flags) override;

  content::RenderWidgetHost* const host_;
};

}  // namespace keyboard_lock

#endif  // COMPONENTS_KEYBOARD_LOCK_WIDGET_KEY_EVENT_FILTER_H_
