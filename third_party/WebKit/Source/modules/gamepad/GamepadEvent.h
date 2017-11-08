// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef GamepadEvent_h
#define GamepadEvent_h

#include "modules/EventModules.h"
#include "modules/gamepad/Gamepad.h"
#include "modules/gamepad/GamepadEventInit.h"

namespace blink {

class GamepadEvent final : public Event {
  DEFINE_WRAPPERTYPEINFO();

 public:
  static GamepadEvent* Create(const AtomicString& type,
                              bool can_bubble,
                              bool cancelable,
                              Gamepad* gamepad) {
    // First get the Time from the system
    base::Time system_time = base::Time::NowFromSystemTime();
    // Now get the Time base for now and also the TimeTicks
    base::Time time_base = base::Time::Now();
    base::TimeTicks time_ticks_base = base::TimeTicks::Now();

    // In order to convert the system type to time ticks we will
    // use the delta between the system time and time_base
    base::TimeDelta time_difference = time_base - system_time;

    // Then the system time in TimeTicks is the difference between
    // time_ticks_base and time_difference.
    base::TimeTicks timestamp = time_ticks_base - time_difference;

    return new GamepadEvent(type, can_bubble, cancelable,
                            WTF::TimeTicks(timestamp), gamepad);
  }
  static GamepadEvent* Create(const AtomicString& type,
                              const GamepadEventInit& initializer) {
    return new GamepadEvent(type, initializer);
  }
  ~GamepadEvent() override;

  Gamepad* getGamepad() const { return gamepad_.Get(); }

  const AtomicString& InterfaceName() const override;

  void Trace(blink::Visitor*) override;

 private:
  GamepadEvent(const AtomicString& type,
               bool can_bubble,
               bool cancelable,
               WTF::TimeTicks time_stamp,
               Gamepad*);
  GamepadEvent(const AtomicString&, const GamepadEventInit&);

  Member<Gamepad> gamepad_;
};

}  // namespace blink

#endif  // GamepadEvent_h
