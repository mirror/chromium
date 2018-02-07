// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/keyboard_lock/keyboard_lock_host.h"

#include <string>
#include <vector>

#include "base/bind.h"
#include "base/logging.h"
#include "base/macros.h"
#include "content/browser/keyboard_lock/keyboard_lock_tab_observer.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/render_widget_host.h"
#include "content/public/browser/render_widget_host_view.h"
#include "content/public/browser/web_contents.h"
#include "ui/events/keycodes/dom/keycode_converter.h"
#include "ui/gfx/native_widget_types.h"

#if defined(USE_AURA)
#include "ui/aura/window.h"
#include "ui/aura/window_tree_host.h"
#endif

namespace content {

// static
std::unique_ptr<KeyboardLockHost> KeyboardLockHost::Create(
    RenderWidgetHostView* host_view,
    const std::vector<std::string>& key_codes) {
  DCHECK(host_view);

  RenderWidgetHost* render_widget_host = host_view->GetRenderWidgetHost();
  if (!render_widget_host)
    return std::unique_ptr<KeyboardLockHost>();

  RenderViewHost* render_view_host = RenderViewHost::From(render_widget_host);
  if (!render_view_host)
    return std::unique_ptr<KeyboardLockHost>();

  WebContents* web_contents = WebContents::FromRenderViewHost(render_view_host);
  if (!web_contents)
    return std::unique_ptr<KeyboardLockHost>();

  std::unique_ptr<KeyboardLockHost> keyboard_lock_host(
      new KeyboardLockHost(host_view, render_widget_host, web_contents));
  keyboard_lock_host->RequestKeyboardLock(key_codes);

  return keyboard_lock_host;
}

KeyboardLockHost::KeyboardLockHost(RenderWidgetHostView* host_view,
                                   RenderWidgetHost* render_widget_host,
                                   WebContents* web_contents)
    : host_view_(host_view) {
  tab_observer_ = std::make_unique<KeyboardLockTabObserver>(
      render_widget_host, web_contents,
      base::BindRepeating(&KeyboardLockHost::UpdateLockState,
                          base::Unretained(this)));
}

KeyboardLockHost::~KeyboardLockHost() {
  UnlockKeyboard();
};

void KeyboardLockHost::RequestKeyboardLock(
    const std::vector<std::string>& codes) {
  for (const std::string& code : codes) {
    native_key_codes_.push_back(
        ui::KeycodeConverter::CodeStringToNativeKeycode(code));
  }

  UpdateLockState();
}

void KeyboardLockHost::LockKeyboard() {
#if defined(USE_AURA)
  aura::Window* window = host_view_->GetNativeView();
  if (!window)
    return;

  aura::WindowTreeHost* window_tree_host = window->GetHost();
  if (window_tree_host) {
    window_tree_host->LockKeys(native_key_codes_);
    keyboard_lock_activated_ = true;
  }
#else
  NOTIMPLEMENTED();
#endif
}

void KeyboardLockHost::UnlockKeyboard() {
#if defined(USE_AURA)
  aura::Window* window = host_view_->GetNativeView();
  if (!window)
    return;

  aura::WindowTreeHost* window_tree_host = window->GetHost();
  if (window_tree_host) {
    window_tree_host->UnlockKeys();
    keyboard_lock_activated_ = false;
  }
#else
  NOTIMPLEMENTED();
#endif
}

void KeyboardLockHost::UpdateLockState() {
  if (!tab_observer_) {
    return;
  }

  if (tab_observer_->is_fullscreen() && tab_observer_->is_focused()) {
    LockKeyboard();
  } else if (keyboard_lock_activated_) {
    UnlockKeyboard();
  }
}

}  // namespace content
