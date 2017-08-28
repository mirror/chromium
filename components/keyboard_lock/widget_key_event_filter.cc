// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/keyboard_lock/widget_key_event_filter.h"

#include "base/logging.h"
#include "content/public/browser/native_web_keyboard_event.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/render_widget_host.h"
#include "content/public/browser/web_contents.h"
#include "ui/events/event.h"

namespace keyboard_lock {

namespace {

content::NativeWebKeyboardEvent CreateNativeEvent(ui::EventType type,
                                                  ui::KeyboardCode code,
                                                  int flags) {
  content::NativeWebKeyboardEvent event(ui::KeyEvent(type, code, flags));
  event.skip_in_browser = true;
  return event;
}

}  // namespace

WidgetKeyEventFilter::WidgetKeyEventFilter(const content::WebContents* contents)
    : host_(contents->GetRenderViewHost()->GetWidget()) {
  DCHECK(host_);
}

WidgetKeyEventFilter::~WidgetKeyEventFilter() = default;

bool WidgetKeyEventFilter::OnKeyDown(ui::KeyboardCode code, int flags) {
  LOG(ERROR) << "Forwarding OnKeyDown " << code << " with flags " << flags;
  host_->ForwardKeyboardEvent(CreateNativeEvent(
      ui::ET_KEY_PRESSED, code, flags));
  return true;
}

bool WidgetKeyEventFilter::OnKeyUp(ui::KeyboardCode code, int flags) {
  LOG(ERROR) << "Forwarding OnKeyUp " << code << " with flags " << flags;
  host_->ForwardKeyboardEvent(CreateNativeEvent(
      ui::ET_KEY_RELEASED, code, flags));
  return true;
}

}  // namespace keyboard_lock
