// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/keyboard_lock/keyboard_lock_host.h"

#include <string>
#include <vector>

#include "base/bind.h"
#include "base/callback.h"
#include "base/location.h"
#include "base/memory/ref_counted.h"
#include "base/memory/singleton.h"
#include "base/single_thread_task_runner.h"
#include "content/browser/renderer_host/render_widget_host_view_event_handler.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/render_widget_host.h"
#include "content/public/browser/web_contents.h"
#include "ui/events/keycodes/dom/keycode_converter.h"

namespace content {

// TODO: RenderWidgetHostViewEventHandler is only used for Aura so update this
// class to work for macOS/non-Aura as well.
KeyboardLockHost::KeyboardLockHost(
    RenderWidgetHost* host,
    RenderWidgetHostViewEventHandler* event_handler)
    : render_widget_host_(host), event_handler_(event_handler) {
  DCHECK(render_widget_host_);
  DCHECK(event_handler_);

  RenderViewHost* render_view_host = RenderViewHost::From(render_widget_host_);
  if (render_view_host) {
    web_contents_ = WebContents::FromRenderViewHost(render_view_host);
  }

  if (web_contents_) {
    // NOTE:
    // WebContentsObserver appears to be the only way to distinguish between tab
    // initiated and browser initiated fullscreen (javascript vs. user action).
    // As specified we only want to enable KBLock for tab/javascript initiated
    // fullscreen.
    tab_observer_ = std::make_unique<KeyboardLockTabObserver>(
        render_widget_host_, web_contents_,
        base::BindRepeating(&KeyboardLockHost::UpdateLockState,
                            base::Unretained(this)));
  } else {
    LOG(ERROR) << "NO WEBCONTENTS!!";
  }

  // NOTE:
  // |web_contents_| will be used by the longpress listener to exit fullscreen
  // mode (none of the classes under Webcontents has the ability to do so via
  // their interface).
}

KeyboardLockHost::~KeyboardLockHost() = default;

void KeyboardLockHost::ReserveKeys(const std::vector<std::string>& codes,
                                   base::OnceCallback<void(bool)> done) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

  std::vector<int> native_codes;
  for (const std::string& code : codes) {
    native_codes.push_back(
        ui::KeycodeConverter::CodeStringToNativeKeycode(code));
  }
  ReserveKeyCodes(native_codes, std::move(done));
}

void KeyboardLockHost::ClearReservedKeys() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

  keyboard_lock_active_ = false;
  UpdateLockState();
}

void KeyboardLockHost::ReserveKeyCodes(const std::vector<int>& key_codes,
                                       base::OnceCallback<void(bool)> done) {
  // TODO(joedow): |keyboard_lock_active_| only indicates whether the site has
  // requested keyboard lock, We will need to pass |key_codes| in the future.
  keyboard_lock_active_ = true;
  UpdateLockState();

  if (done) {
    std::move(done).Run(true);
  }
}

void KeyboardLockHost::UpdateLockState() {
  if (!tab_observer_) {
    return;
  }

  if (keyboard_lock_active_ && tab_observer_->is_fullscreen() &&
      tab_observer_->is_focused()) {
    event_handler_->ReserveKeys();
  } else {
    event_handler_->ClearReservedKeys();
  }
}

}  // namespace content
