// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/keyboard_lock/client.h"

#include "content/public/browser/native_web_keyboard_event.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/render_widget_host.h"
#include "content/public/browser/web_contents.h"

namespace content {
namespace keyboard_lock {

Client::Client(content::WebContents* web_contents)
    : host_(web_contents->GetRenderViewHost()->GetWidget()) {
  DCHECK(host_);
}

Client::~Client() = default;

void Client::OnKeyEvent(const ui::KeyEvent& event) {
  content::NativeWebKeyboardEvent web_event(event);
  web_event.skip_in_browser = true;
  host_->ForwardKeyboardEvent(web_event);
}

}  // namespace keyboard_lock
}  // namespace content
