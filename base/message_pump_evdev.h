// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_MESSAGE_PUMP_EVDEV_H
#define BASE_MESSAGE_PUMP_EVDEV_H

#if !defined(MESSAGE_PUMP_FOR_UI)
#define MESSAGE_PUMP_EVDEV_FOR_UI
#define MESSAGE_PUMP_FOR_UI
#endif

#include "base/message_pump_epoll.h"

namespace base {

enum EvdevEventType {
  EVDEV_EVENT_RELATIVE_MOTION,
  EVDEV_EVENT_BUTTON,
  EVDEV_EVENT_KEY,
};

struct EvdevMotionEvent {
  int x, y;
};

struct EvdevEvent {
  EvdevEventType type;
  timeval time;
  int modifiers;
  int code, value;
  int x, y;
  union {
    EvdevMotionEvent motion;
  };
};

// This class implements a message-pump for processing events from input devices
// Refer to MessagePump for further documentation
class BASE_EXPORT MessagePumpEvdev : public MessagePumpEpoll {
 public:
  MessagePumpEvdev();
  virtual ~MessagePumpEvdev();

 private:
  MessagePumpEpollHandler* udev_handler_;

  DISALLOW_COPY_AND_ASSIGN(MessagePumpEvdev);
};

#if defined(MESSAGE_PUMP_EVDEV_FOR_UI)
typedef MessagePumpEvdev MessagePumpForUI;
#endif

}  // namespace base

#endif  // BASE_MESSAGE_PUMP_EVDEV_H
