// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/keyboard_lock/platform_key_hook.h"

#include <X11/Xlib.h>
#undef Status

#include <unordered_set>
#include <vector>

#include "base/logging.h"
#include "base/macros.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_window.h"
#include "components/keyboard_lock/platform_key_event_filter.h"
#include "ui/aura/window.h"
#include "ui/aura/window_tree_host.h"
#include "ui/base/x/x11_window_event_manager.h"
#include "ui/events/platform/x11/x11_event_source.h"
#include "ui/gfx/x/x11_error_tracker.h"
#include "ui/gfx/x/x11_types.h"

namespace keyboard_lock {

namespace {

const unsigned int kSingleStates[] = { ShiftMask,
                                       LockMask,
                                       ControlMask,
                                       Mod1Mask,
                                       Mod2Mask,
                                       Mod3Mask,
                                       Mod4Mask,
                                       Mod5Mask };

std::vector<int> all_states;

void InitializeStates() {
  if (!all_states.empty()) {
    return;
  }

  std::unordered_set<int> states {
      kSingleStates, kSingleStates + arraysize(kSingleStates) };
  for (size_t i = 0; i < arraysize(kSingleStates); i++) {
    std::vector<int> new_states;
    for (auto state : states) {
      for (size_t j = 0; j < arraysize(kSingleStates); j++) {
        new_states.push_back(state | kSingleStates[j]);
      }
    }
    states.insert(new_states.begin(), new_states.end());
  }

  all_states = std::vector<int> { states.begin(), states.end() };
}

}  // namespace

PlatformKeyHook::PlatformKeyHook(Browser* owner, KeyEventFilter* filter)
    : owner_(owner),
      filter_(filter) {
  DCHECK(owner_);
  ui::X11EventSource::GetInstance()->set_platform_key_event_filter(&filter_);
}

PlatformKeyHook::~PlatformKeyHook() = default;

bool PlatformKeyHook::RegisterKey(const std::vector<ui::KeyboardCode>& codes) {
  InitializeStates();
  gfx::X11ErrorTracker x11_error_tracker;
  Display* display = gfx::GetXDisplay();
  if (owner_->window() == nullptr ||
      owner_->window()->GetNativeWindow() == nullptr ||
      owner_->window()->GetNativeWindow()->GetHost() == nullptr) {
    LOG(ERROR) << "No native window found.";
    // This should not happen in production, JavaScript cannot be executed
    // before CreateBrowserWindow().
    return false;
  }
  const Window root =
      owner_->window()->GetNativeWindow()->GetHost()->GetAcceleratedWidget();
  for (ui::KeyboardCode code : codes) {
    for (auto state : all_states) {
      XGrabKey(display, code, state, root, False, GrabModeAsync, GrabModeAsync);
    }
  }
  return true;
}

bool PlatformKeyHook::UnregisterKey(
    const std::vector<ui::KeyboardCode>& codes) {
  InitializeStates();
  gfx::X11ErrorTracker x11_error_tracker;
  Display* display = gfx::GetXDisplay();
  const Window root = DefaultRootWindow(display);
  for (ui::KeyboardCode code : codes) {
    for (auto state : all_states) {
      XUngrabKey(display, code, state, root);
    }
  }
  return true;
}

}  // namespace keyboard_lock
