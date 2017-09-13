// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/keyboard_lock/client.h"

#include "base/memory/ptr_util.h"
#include "content/public/browser/native_web_keyboard_event.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/render_widget_host.h"
#include "content/public/browser/web_contents.h"

namespace content {
namespace keyboard_lock {

Client::Client(content::WebContents* web_contents)
    : contents_(web_contents),
      host_(web_contents->GetRenderViewHost()->GetWidget()) {
  DCHECK(contents_);
  DCHECK(host_);
}

Client::~Client() = default;

// static
std::unique_ptr<ui::aura::keyboard_lock::Client> Client::Create(
    content::WebContents* web_contents) {
  std::unique_ptr<Client> client(new Client(web_contents));
  ui::aura::keyboard_lock::LongPressDetector::Delegate* delegate = client.get();
  return base::MakeUnique<ui::aura::keyboard_lock::LongPressDetector>(
      1, 2000, delegate, std::move(client));
}

void Client::OnKeyEvent(const ui::KeyEvent& event) {
  content::NativeWebKeyboardEvent web_event(event);
  web_event.skip_in_browser = true;
  host_->ForwardKeyboardEvent(web_event);
}

void Client::LongPressDetected() {
  contents_->ExitFullscreen(true);
}

}  // namespace keyboard_lock
}  // namespace content
