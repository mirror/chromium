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
#include "ui/base/keyboard_lock/platform_hook.h"
#include "ui/events/keycodes/dom/keycode_converter.h"

namespace content {

KeyboardLockHost::KeyboardLockHost(RenderWidgetHostView* host_view)
    : host_view_(host_view), platform_hook_(ui::PlatformHook::Create()) {
  DCHECK(host_view_);
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

  platform_hook_.reset();
  active_key_filter_.reset();
}

void KeyboardLockHost::ReserveKeyCodes(const std::vector<int>& key_codes,
                                       base::OnceCallback<void(bool)> done) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

  if (!active_key_filter_) {
    active_key_filter_.reset(new ActiveKeyFilter(host_view_));
  }

  // TODO(joedow): |active_key_filter_| always returns true for all keys now,
  // however we will need to pass |key_codes| in later on.

  if (activated_) {
    platform_hook_->Register(active_key_filter_.get());
  }

  if (done) {
    std::move(done).Run(true);
  }
}

void KeyboardLockHost::Activate() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

  activated_ = true;

  if (active_key_filter_) {
    platform_hook_->Register(active_key_filter_.get());
  }
}

void KeyboardLockHost::Deactivate() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

  activated_ = false;
  platform_hook_->Unregister();
}

}  // namespace content
