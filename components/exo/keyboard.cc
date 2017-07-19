// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/exo/keyboard.h"

#include "base/threading/thread_task_runner_handle.h"
#include "components/exo/keyboard_delegate.h"
#include "components/exo/keyboard_device_configuration_delegate.h"
#include "components/exo/shell_surface.h"
#include "components/exo/surface.h"
#include "ui/aura/client/focus_client.h"
#include "ui/aura/window.h"
#include "ui/base/ime/input_method.h"
#include "ui/events/base_event_utils.h"
#include "ui/events/devices/input_device.h"
#include "ui/events/devices/input_device_manager.h"
#include "ui/events/event.h"
#include "ui/views/widget/widget.h"

namespace exo {
namespace {

constexpr base::TimeDelta kExpirationTimeForPendingKeyAcks =
    base::TimeDelta::FromMilliseconds(5000);

views::Widget* GetTopLevelWidgetForSurface(Surface* surface) {
  return surface
             ? views::Widget::GetTopLevelWidgetForNativeView(surface->window())
             : nullptr;
}

bool ProcessAccelerator(Surface* surface, ui::KeyEvent* event) {
  views::Widget* widget = GetTopLevelWidgetForSurface(surface);
  if (widget) {
    views::FocusManager* focus_manager = widget->GetFocusManager();
    return focus_manager->ProcessAccelerator(ui::Accelerator(*event));
  }
  return false;
}

bool ConsumedByIme(Surface* focus, const ui::KeyEvent* event) {
  // Check if IME consumed the event, to avoid it to be doubly processed.
  // First let us see whether IME is active and is in text input mode.
  views::Widget* widget = GetTopLevelWidgetForSurface(focus);
  ui::InputMethod* ime = widget ? widget->GetInputMethod() : nullptr;
  if (!ime || ime->GetTextInputType() == ui::TEXT_INPUT_TYPE_NONE)
    return false;

  // Case 1:
  // When IME ate a key event but did not emit character insertion event yet
  // (e.g., when it is still showing a candidate list UI to the user,) the
  // consumed key event is re-sent after masked |key_code| by VKEY_PROCESSKEY.
  if (event->key_code() == ui::VKEY_PROCESSKEY)
    return true;

  // Case 2:
  // When IME ate a key event and generated a single character input, it leaves
  // the key event as-is, and in addition calls the active ui::TextInputClient's
  // InsertChar() method. (In our case, arc::ArcImeService::InsertChar()).
  //
  // In Chrome OS (and Web) convention, the two calls wont't cause duplicates,
  // because key-down events do not mean any character inputs there.
  // (InsertChar issues a DOM "keypress" event, which is distinct from keydown.)
  // Unfortunately, this is not necessary the case for our clients that may
  // treat keydown as a trigger of text inputs. We need suppression for keydown.
  if (event->type() == ui::ET_KEY_PRESSED) {
    // Same condition as components/arc/ime/arc_ime_service.cc#InsertChar.
    const base::char16 ch = event->GetCharacter();
    const bool is_control_char =
        (0x00 <= ch && ch <= 0x1f) || (0x7f <= ch && ch <= 0x9f);
    if (!is_control_char && !ui::IsSystemKeyModifier(event->flags()))
      return true;
  }

  // Case 3:
  // Workaround for apps that doesn't handle hardware keyboard events well.
  // Keys typically on software keyboard and lack of them are fatal, namely,
  // unmodified enter and backspace keys, are sent through IME.
  constexpr int kModifierMask = ui::EF_SHIFT_DOWN | ui::EF_CONTROL_DOWN |
                                ui::EF_ALT_DOWN | ui::EF_COMMAND_DOWN |
                                ui::EF_ALTGR_DOWN | ui::EF_MOD3_DOWN;
  // Same condition as components/arc/ime/arc_ime_service.cc#InsertChar.
  if ((event->flags() & kModifierMask) == 0) {
    if (event->key_code() == ui::VKEY_RETURN ||
        event->key_code() == ui::VKEY_BACK) {
      return true;
    }
  }

  return false;
}

bool IsPhysicalKeyboardEnabled() {
  // The internal keyboard is enabled if maximize mode is not enabled.
  if (!WMHelper::GetInstance()->IsMaximizeModeWindowManagerEnabled())
    return true;

  for (auto& keyboard :
       ui::InputDeviceManager::GetInstance()->GetKeyboardDevices()) {
    if (keyboard.type != ui::InputDeviceType::INPUT_DEVICE_INTERNAL)
      return true;
  }
  return false;
}

}  // namespace

////////////////////////////////////////////////////////////////////////////////
// Keyboard, public:

Keyboard::Keyboard(KeyboardDelegate* delegate)
    : delegate_(delegate), weak_factory_(this) {
  auto* helper = WMHelper::GetInstance();
  helper->AddPostTargetHandler(this);
  helper->AddFocusObserver(this);
  helper->AddMaximizeModeObserver(this);
  helper->AddInputDeviceEventObserver(this);
  OnWindowFocused(helper->GetFocusedWindow(), nullptr);
}

Keyboard::~Keyboard() {
  for (KeyboardObserver& observer : observer_list_)
    observer.OnKeyboardDestroying(this);
  if (focus_)
    focus_->RemoveSurfaceObserver(this);
  auto* helper = WMHelper::GetInstance();
  helper->RemoveFocusObserver(this);
  helper->RemovePostTargetHandler(this);
  helper->RemoveMaximizeModeObserver(this);
  helper->RemoveInputDeviceEventObserver(this);
}

bool Keyboard::HasDeviceConfigurationDelegate() const {
  return !!device_configuration_delegate_;
}

void Keyboard::SetDeviceConfigurationDelegate(
    KeyboardDeviceConfigurationDelegate* delegate) {
  device_configuration_delegate_ = delegate;
  OnKeyboardDeviceConfigurationChanged();
}

void Keyboard::AddObserver(KeyboardObserver* observer) {
  observer_list_.AddObserver(observer);
}

bool Keyboard::HasObserver(KeyboardObserver* observer) const {
  return observer_list_.HasObserver(observer);
}

void Keyboard::RemoveObserver(KeyboardObserver* observer) {
  observer_list_.HasObserver(observer);
}

void Keyboard::SetNeedKeyboardKeyAcks(bool need_acks) {
  are_keyboard_key_acks_needed_ = need_acks;
}

bool Keyboard::AreKeyboardKeyAcksNeeded() const {
  return are_keyboard_key_acks_needed_;
}

void Keyboard::AckKeyboardKey(uint32_t serial, bool handled) {
  auto iter = pending_key_acks_.find(serial);
  if (iter == pending_key_acks_.end())
    return;

  if (!handled)
    ProcessAccelerator(focus_, &iter->second);
  pending_key_acks_.erase(serial);
}

////////////////////////////////////////////////////////////////////////////////
// ui::EventHandler overrides:

void Keyboard::OnKeyEvent(ui::KeyEvent* event) {
  // Pass accelerators to ShellSurfaceWidget before sending it to Android if ack
  // key event is not needed.
  if (!AreKeyboardKeyAcksNeeded()) {
    if (ProcessAccelerator(focus_, event)) {
      event->StopPropagation();
      return;
    }
  }

  // These modifiers reflect what Wayland is aware of.  For example,
  // EF_SCROLL_LOCK_ON is missing because Wayland doesn't support scroll lock.
  const int kModifierMask = ui::EF_SHIFT_DOWN | ui::EF_CONTROL_DOWN |
                            ui::EF_ALT_DOWN | ui::EF_COMMAND_DOWN |
                            ui::EF_ALTGR_DOWN | ui::EF_MOD3_DOWN |
                            ui::EF_NUM_LOCK_ON | ui::EF_CAPS_LOCK_ON;
  int modifier_flags = event->flags() & kModifierMask;
  if (modifier_flags != modifier_flags_) {
    modifier_flags_ = modifier_flags;
    if (focus_)
      delegate_->OnKeyboardModifiers(modifier_flags_);
  }

  // When IME ate a key event, we use the event only for tracking key states and
  // ignore for further processing. Otherwise it is handled in two places (IME
  // and client) and causes undesired behavior.
  bool consumed_by_ime = ConsumedByIme(focus_, event);

  switch (event->type()) {
    case ui::ET_KEY_PRESSED: {
      auto it =
          std::find(pressed_keys_.begin(), pressed_keys_.end(), event->code());
      if (it == pressed_keys_.end()) {
        if (focus_ && !consumed_by_ime) {
          uint32_t serial = delegate_->OnKeyboardKey(event->time_stamp(),
                                                     event->code(), true);
          if (are_keyboard_key_acks_needed_)
            RecordSentKey(serial, event);
        }

        pressed_keys_.push_back(event->code());
      }
    } break;
    case ui::ET_KEY_RELEASED: {
      auto it =
          std::find(pressed_keys_.begin(), pressed_keys_.end(), event->code());
      if (it != pressed_keys_.end()) {
        if (focus_ && !consumed_by_ime) {
          uint32_t serial = delegate_->OnKeyboardKey(event->time_stamp(),
                                                     event->code(), false);
          if (are_keyboard_key_acks_needed_)
            RecordSentKey(serial, event);
        }

        pressed_keys_.erase(it);
      }
    } break;
    default:
      NOTREACHED();
      break;
  }
}

////////////////////////////////////////////////////////////////////////////////
// aura::client::FocusChangeObserver overrides:

void Keyboard::OnWindowFocused(aura::Window* gained_focus,
                               aura::Window* lost_focus) {
  Surface* gained_focus_surface =
      gained_focus ? GetEffectiveFocus(gained_focus) : nullptr;
  if (gained_focus_surface != focus_) {
    if (focus_) {
      delegate_->OnKeyboardLeave(focus_);
      focus_->RemoveSurfaceObserver(this);
      focus_ = nullptr;
      pending_key_acks_.clear();
    }
    if (gained_focus_surface) {
      delegate_->OnKeyboardModifiers(modifier_flags_);
      delegate_->OnKeyboardEnter(gained_focus_surface, pressed_keys_);
      focus_ = gained_focus_surface;
      focus_->AddSurfaceObserver(this);
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
// SurfaceObserver overrides:

void Keyboard::OnSurfaceDestroying(Surface* surface) {
  DCHECK(surface == focus_);
  focus_ = nullptr;
  surface->RemoveSurfaceObserver(this);
}

////////////////////////////////////////////////////////////////////////////////
// ui::InputDeviceEventObserver overrides:

void Keyboard::OnKeyboardDeviceConfigurationChanged() {
  if (device_configuration_delegate_) {
    device_configuration_delegate_->OnKeyboardTypeChanged(
        IsPhysicalKeyboardEnabled());
  }
}

////////////////////////////////////////////////////////////////////////////////
// WMHelper::MaximizeModeObserver overrides:

void Keyboard::OnMaximizeModeStarted() {
  OnKeyboardDeviceConfigurationChanged();
}

void Keyboard::OnMaximizeModeEnding() {}

void Keyboard::OnMaximizeModeEnded() {
  OnKeyboardDeviceConfigurationChanged();
}

////////////////////////////////////////////////////////////////////////////////
// Keyboard, private:

Surface* Keyboard::GetEffectiveFocus(aura::Window* window) const {
  // Use window surface as effective focus.
  Surface* focus = Surface::AsSurface(window);
  if (!focus) {
    // Fallback to main surface.
    aura::Window* top_level_window = window->GetToplevelWindow();
    if (top_level_window)
      focus = ShellSurface::GetMainSurface(top_level_window);
  }

  return focus && delegate_->CanAcceptKeyboardEventsForSurface(focus) ? focus
                                                                      : nullptr;
}

void Keyboard::RecordSentKey(uint32_t serial, ui::KeyEvent* event) {
  ProcessExpiredPendingKeyAcks();
  pending_key_acks_.insert({serial, ui::KeyEvent(*event)});
  if (!weak_factory_.HasWeakPtrs()) {
    base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(
        FROM_HERE,
        base::BindOnce(&Keyboard::ProcessExpiredPendingKeyAcks,
                       weak_factory_.GetWeakPtr()),
        kExpirationTimeForPendingKeyAcks);
  }
}

void Keyboard::ProcessExpiredPendingKeyAcks() {
  weak_factory_.InvalidateWeakPtrs();

  base::TimeTicks now = base::TimeTicks::Now();
  base::TimeDelta delta_to_next_expired = kExpirationTimeForPendingKeyAcks;

  // Check all pending acks and process them as if it's not handled if
  // expiration time passed.
  auto iter = pending_key_acks_.begin();
  while (iter != pending_key_acks_.end()) {
    ui::KeyEvent& event = iter->second;
    base::TimeDelta delta = now - event.time_stamp();
    if (delta > kExpirationTimeForPendingKeyAcks) {
      ProcessAccelerator(focus_, &event);
      pending_key_acks_.erase(iter++);
    } else {
      delta_to_next_expired = std::min(
          delta_to_next_expired, kExpirationTimeForPendingKeyAcks - delta);
      iter++;
    }
  }

  if (!pending_key_acks_.empty()) {
    base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(
        FROM_HERE,
        base::BindOnce(&Keyboard::ProcessExpiredPendingKeyAcks,
                       weak_factory_.GetWeakPtr()),
        kExpirationTimeForPendingKeyAcks);
  }
}

}  // namespace exo
