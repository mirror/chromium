// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/logging.h"
#include "base/message_pump_evdev.h"
#include "ui/aura/root_window.h"
#include "ui/aura/ui_controls_aura.h"
#include "ui/ui_controls/ui_controls_aura.h"

namespace aura {
namespace {

// Event waiter executes the specified closure|when a matching event
// is found.
// TODO(oshima): Move this to base.
class EventWaiter : public MessageLoopForUI::Observer {
 public:
  typedef bool (*EventWaiterMatcher)(const base::NativeEvent& event);

  EventWaiter(const base::Closure& closure, EventWaiterMatcher matcher)
      : closure_(closure),
        matcher_(matcher) {
    MessageLoopForUI::current()->AddObserver(this);
  }

  virtual ~EventWaiter() {
    MessageLoopForUI::current()->RemoveObserver(this);
  }

  // MessageLoop::Observer implementation:
  virtual base::EventStatus WillProcessEvent(
      const base::NativeEvent& event) OVERRIDE {
    if ((*matcher_)(event)) {
      MessageLoop::current()->PostTask(FROM_HERE, closure_);
      delete this;
    }
    return base::EVENT_CONTINUE;
  }

  virtual void DidProcessEvent(const base::NativeEvent& event) OVERRIDE {
  }

 private:
  base::Closure closure_;
  EventWaiterMatcher matcher_;
  DISALLOW_COPY_AND_ASSIGN(EventWaiter);
};

// Returns true when the event is a marker event.
bool Matcher(const base::NativeEvent& event) {
  NOTIMPLEMENTED();
  return false;
}

class UIControlsDRM : public ui_controls::UIControlsAura {
 public:
  UIControlsDRM(RootWindow* root_window) : root_window_(root_window) {
  }

  virtual bool SendKeyPress(gfx::NativeWindow window,
                            ui::KeyboardCode key,
                            bool control,
                            bool shift,
                            bool alt,
                            bool command) {
    DCHECK(!command);  // No command key on Aura
    return SendKeyPressNotifyWhenDone(
        window, key, control, shift, alt, command, base::Closure());
  }
  virtual bool SendKeyPressNotifyWhenDone(gfx::NativeWindow window,
                                          ui::KeyboardCode key,
                                          bool control,
                                          bool shift,
                                          bool alt,
                                          bool command,
                                          const base::Closure& closure) {
    NOTIMPLEMENTED();
    RunClosureAfterAllPendingUIEvents(closure);
    return true;
  }

  // Simulate a mouse move. (x,y) are absolute screen coordinates.
  virtual bool SendMouseMove(long x, long y) {
    return SendMouseMoveNotifyWhenDone(x, y, base::Closure());
  }
  virtual bool SendMouseMoveNotifyWhenDone(long x,
                                           long y,
                                           const base::Closure& closure) {
    NOTIMPLEMENTED();
    return true;
  }
  virtual bool SendMouseEvents(ui_controls::MouseButton type, int state) {
    return SendMouseEventsNotifyWhenDone(type, state, base::Closure());
  }
  virtual bool SendMouseEventsNotifyWhenDone(ui_controls::MouseButton type,
                                             int state,
                                             const base::Closure& closure) {
    NOTIMPLEMENTED();
    return true;
  }
  virtual bool SendMouseClick(ui_controls::MouseButton type) {
    return SendMouseEvents(type, ui_controls::UP | ui_controls::DOWN);
  }
  virtual void RunClosureAfterAllPendingUIEvents(const base::Closure& closure) {
    if (closure.is_null())
      return;
    new EventWaiter(closure, &Matcher);
  }

 private:
  RootWindow* root_window_;
  DISALLOW_COPY_AND_ASSIGN(UIControlsDRM);
};

}  // namespace

ui_controls::UIControlsAura* CreateUIControlsAura(RootWindow* root_window) {
  return new UIControlsDRM(root_window);
}

}  // namespace aura
