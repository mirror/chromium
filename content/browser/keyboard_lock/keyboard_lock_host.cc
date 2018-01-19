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
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/render_widget_host.h"
#include "content/public/browser/web_contents.h"
#include "ui/base/keyboard_lock/platform_hook.h"
#include "ui/events/keycodes/dom/keycode_converter.h"

namespace content {

KeyboardLockHost::KeyboardLockHost(RenderWidgetHost* host)
    : render_widget_host_(host), platform_hook_(ui::PlatformHook::Create()) {
  DCHECK(render_widget_host_);
  DCHECK(platform_hook_);

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

  active_key_filter_.reset();
  UpdateLockState();
}

void KeyboardLockHost::ReserveKeyCodes(const std::vector<int>& key_codes,
                                       base::OnceCallback<void(bool)> done) {
  // TODO(joedow): |active_key_filter_| always returns true for all keys now,
  // however we will need to pass |key_codes| in later on.  We will need to
  // update the list of keys in the filter once this functionality exists.
  if (!active_key_filter_) {
    active_key_filter_.reset(new ActiveKeyFilter(render_widget_host_));
  }
  UpdateLockState();

  if (done) {
    std::move(done).Run(true);
  }
}

void KeyboardLockHost::UpdateLockState() {
  if (!tab_observer_) {
    return;
  }

  if (tab_observer_->is_fullscreen() && tab_observer_->is_focused() &&
      active_key_filter_) {
    platform_hook_->Register(active_key_filter_.get());
  } else {
    platform_hook_->Unregister();
  }
}

}  // namespace content
