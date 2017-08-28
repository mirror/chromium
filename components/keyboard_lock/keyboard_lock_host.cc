// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/keyboard_lock/keyboard_lock_host.h"

#include <utility>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "base/single_thread_task_runner.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "components/keyboard_lock/active_key_event_filter.h"
#include "components/keyboard_lock/active_key_event_filter_registrar.h"
#include "components/keyboard_lock/key_event_filter_share_wrapper.h"
#include "components/keyboard_lock/key_event_filter_thread_proxy.h"
#include "components/keyboard_lock/key_hook_share_wrapper.h"
#include "components/keyboard_lock/key_hook_state_keeper.h"
#include "components/keyboard_lock/page_observer.h"
#include "components/keyboard_lock/platform_key_hook.h"
#include "components/keyboard_lock/widget_key_event_filter.h"
#include "content/public/browser/web_contents.h"
#include "ui/events/keycodes/dom/keycode_converter.h"

namespace keyboard_lock {

namespace {

std::unique_ptr<KeyHookStateKeeper> CreateKeyHookStateKeeper(
    const scoped_refptr<base::SingleThreadTaskRunner>& runner,
    ActiveKeyEventFilterTracker* tracker,
    Browser* owner,
    KeyEventFilter* filter) {
  auto thread_proxy = base::MakeUnique<KeyEventFilterThreadProxy>(
      runner, base::MakeUnique<ActiveKeyEventFilter>(tracker));
  return base::MakeUnique<KeyHookStateKeeper>(
      std::move(thread_proxy),
      base::MakeUnique<PlatformKeyHook>(owner, filter));
}

}  // namespace

KeyboardLockHost::KeyboardLockHost(
    Browser* owner,
    scoped_refptr<base::SingleThreadTaskRunner> runner)
    : owner_(owner),
      runner_(std::move(runner)),
      key_hook_(CreateKeyHookStateKeeper(
          runner_, &active_key_event_filter_tracker_, owner_, filter())) {
  DCHECK(runner_);
  key_hook_.Activate(base::Callback<void(bool)>());
}

KeyboardLockHost::~KeyboardLockHost() = default;

// static
KeyboardLockHost* KeyboardLockHost::Find(const content::WebContents* contents) {
  Browser* browser = chrome::FindBrowserWithWebContents(contents);
  if (!browser) {
    LOG(ERROR) << "No browser found from WebContents " << contents;
    return nullptr;
  }
  DCHECK(browser->GetKeyboardLockHost());
  return browser->GetKeyboardLockHost();
}

KeyEventFilter* KeyboardLockHost::filter() const {
  return const_cast<KeyHookThreadWrapper*>(&key_hook_);
}

void KeyboardLockHost::SetReservedKeys(
    content::WebContents* contents,
    const std::vector<ui::KeyboardCode>& codes,
    base::Callback<void(bool)> on_result) {
  if (!runner_->BelongsToCurrentThread()) {
    // TODO(zijiehe): The |contents| is not safe to be posted to other threads.
    if (!runner_->PostTask(FROM_HERE, base::BindOnce(
            &KeyboardLockHost::SetReservedKeys,
            base::Unretained(this),
            base::Unretained(contents),
            codes,
            on_result))) {
      if (on_result) {
        on_result.Run(false);
      }
    }
    return;
  }

  KeyHookActivator* key_hook = key_hooks_.Find(contents);
  if (key_hook) {
    key_hook->RegisterKey(codes, on_result);
    return;
  }

  auto state_keeper = base::MakeUnique<KeyHookStateKeeper>(
      base::MakeUnique<WidgetKeyEventFilter>(contents),
      base::MakeUnique<KeyHookShareWrapper>(&key_hook_));
  state_keeper->RegisterKey(codes, on_result);
  auto* filter = state_keeper.get();
  key_hooks_.Insert(contents, base::MakeUnique<ActiveKeyEventFilterRegistrar>(
      &active_key_event_filter_tracker_,
      std::move(state_keeper),
      filter));
  PageObserver::Observe(contents, &key_hooks_);
}

void KeyboardLockHost::SetReservedKeyCodes(
    content::WebContents* contents,
    const std::vector<std::string>& codes,
    base::Callback<void(bool)> on_result) {
  std::vector<ui::KeyboardCode> ui_codes;
  for (auto code : codes) {
    ui_codes.push_back(NativeKeycodeToKeyboardCode(
        ui::KeycodeConverter::CodeStringToNativeKeycode(code)));
  }
  SetReservedKeys(contents, ui_codes, std::move(on_result));
}

void KeyboardLockHost::ClearReservedKeys(content::WebContents* contents,
                                         base::Callback<void(bool)> on_result) {
  if (!runner_->BelongsToCurrentThread()) {
    // TODO(zijiehe): The |contents| is not safe to be posted to other threads.
    if (!runner_->PostTask(FROM_HERE, base::BindOnce(
            &KeyboardLockHost::ClearReservedKeys,
            base::Unretained(this),
            base::Unretained(contents),
            on_result))) {
      if (on_result) {
        on_result.Run(false);
      }
    }
    return;
  }

  key_hooks_.Erase(contents);
}

}  // namespace keyboard_lock
