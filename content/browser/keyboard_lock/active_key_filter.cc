// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/keyboard_lock/active_key_filter.h"

#include "content/public/browser/native_web_keyboard_event.h"
#include "content/public/browser/render_widget_host.h"
#include "ui/events/event.h"
#include "ui/events/platform/key_event_filter.h"

namespace content {

ActiveKeyFilter::ActiveKeyFilter(RenderWidgetHost* host) : host_(host) {}

ActiveKeyFilter::~ActiveKeyFilter() = default;

bool ActiveKeyFilter::OnKeyEvent(const ui::KeyEvent& event) {
  content::NativeWebKeyboardEvent web_event(event);
  web_event.skip_in_browser = true;
  host_->ForwardKeyboardEvent(web_event);
  return true;
}

}  // namespace content
