// Copyright (c) 2010 Intel Corporation
// Copyright (c) 2012 The Chromium Authors. All rights reserved.
//
// Permission to use, copy, modify, distribute, and sell this software and
// its documentation for any purpose is hereby granted without fee, provided
// that the above copyright notice appear in all copies and that both that
// copyright notice and this permission notice appear in supporting
// documentation, and that the name of the copyright holders not be used in
// advertising or publicity pertaining to distribution of the software
// without specific, written prior permission.  The copyright holders make
// no representations about the suitability of this software for any
// purpose.  It is provided "as is" without express or implied warranty.
//
// THE COPYRIGHT HOLDERS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS
// SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
// FITNESS, IN NO EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY
// SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER
// RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF
// CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
// CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
//
// Based on: http://cgit.freedesktop.org/wayland/weston/tree/src/evdev.c
//           Commit eb866cd9fe5939efed532f4eb400cd889c00c2f4
//                  "drm: Fix drmModeRes leak on error paths"

#include <stdint.h>
#include <string.h>
#include <libudev.h>
#include <linux/input.h>
#include <unistd.h>
#include <fcntl.h>

#include <vector>
#include <map>
#include <string>

#include "base/message_pump_evdev.h"
#include "ui/base/events.h"

// event type flags
#define EVDEV_ABSOLUTE_DOWN        (1 << 0)
#define EVDEV_ABSOLUTE_UP          (1 << 1)
#define EVDEV_RELATIVE_MOTION      (1 << 2)
#define EVDEV_RELATIVE_SCROLL      (1 << 3)

namespace base {

// Base class for evdev handlers of different device types.
class EvdevHandler : public MessagePumpEpollHandler {
 public:
  EvdevHandler(MessagePumpEpoll *pump, const char *devnode,
               const size_t ev_buf_len);
  virtual ~EvdevHandler();

  void Process();
  virtual bool Configure() = 0;

  char *devnode() { return devnode_; }

 protected:
  virtual void ProcessEvents(input_event *e) = 0;
  virtual void Notify(timeval time, uint16_t code, uint32_t value,
                      EvdevEventType type);
  virtual void NotifyRelMotion(timeval time, uint32_t dx, uint32_t dy);
  virtual void NotifyScroll(timeval time, uint32_t sx, uint32_t sy);
  char *devnode_;
  const size_t ev_buf_len_;
  input_event *ev_buf_;

  DISALLOW_COPY_AND_ASSIGN(EvdevHandler);
};

EvdevHandler::EvdevHandler(MessagePumpEpoll *pump, const char *devnode,
                           const size_t ev_buf_len)
    : MessagePumpEpollHandler(pump), ev_buf_len_(ev_buf_len) {
  fd_ = open(devnode, O_RDONLY);
  DCHECK_GE(fd_, 0);
  devnode_ = strdup(devnode);
  ev_buf_ = new input_event[ev_buf_len_];
}

EvdevHandler::~EvdevHandler() {
  free(devnode_);
  delete[] ev_buf_;
}

void EvdevHandler::Process() {
  input_event *e, *end;
  int len;

  len = read(fd(), ev_buf_, ev_buf_len_ * sizeof e[0]);
  if (len < 0 || len % sizeof e[0] != 0)
    return;

  end = ev_buf_ + (len / sizeof e[0]);
  for (e = ev_buf_; e < end; ++e)
    ProcessEvents(e);
}

void EvdevHandler::Notify(timeval time, uint16_t code, uint32_t value,
                          EvdevEventType type) {
  EvdevEvent event;

  event.id = (uintptr_t) this;
  event.type = type;
  event.time = time;
  event.code = code;
  event.value = value;
  pump_->DispatchEvent(&event);
}

void EvdevHandler::NotifyRelMotion(timeval time, uint32_t dx, uint32_t dy) {
  EvdevEvent event;

  event.id = (uintptr_t) this;
  event.type = EVDEV_EVENT_RELATIVE_MOTION;
  event.time = time;
  event.motion.x = dx;
  event.motion.y = dy;
  pump_->DispatchEvent(&event);
}

void EvdevHandler::NotifyScroll(timeval time, uint32_t sx, uint32_t sy) {
  EvdevEvent event;

  event.id = (uintptr_t) this;
  event.type = EVDEV_EVENT_SCROLL;
  event.time = time;
  event.scroll.x = sx;
  event.scroll.y = sy;
  pump_->DispatchEvent(&event);
}

namespace {

// copied from udev/extras/input_id/input_id.c
// we must use this kernel-compatible implementation
#define BITS_PER_LONG (sizeof(unsigned long) * 8)
#define NBITS(x) ((((x)-1)/BITS_PER_LONG)+1)
#define OFF(x)  ((x)%BITS_PER_LONG)
#define BIT(x)  (1UL<<OFF(x))
#define LONG(x) ((x)/BITS_PER_LONG)
#define TEST_BIT(array, bit)    ((array[LONG(bit)] >> OFF(bit)) & 1)
// end copied

class KeyboardEvdevHandler : public EvdevHandler {
 public:
  KeyboardEvdevHandler(MessagePumpEpoll *pump, const char *devnode);
  virtual ~KeyboardEvdevHandler() {};

  virtual bool Configure() OVERRIDE;

 private:
  static const size_t BUF_LEN = 16;
  struct input_device {
    bool is_repeatable;
  };

  virtual void ProcessEvents(input_event *e) OVERRIDE;
  void ProcessKey(input_event *e);
  input_device device_;

  DISALLOW_COPY_AND_ASSIGN(KeyboardEvdevHandler);
};

bool KeyboardEvdevHandler::Configure() {
  unsigned long ev_bits[NBITS(EV_CNT)];
  ioctl(fd_, EVIOCGBIT(0, sizeof(ev_bits)), ev_bits);
  if (TEST_BIT(ev_bits, EV_REP))
    device_.is_repeatable = true;
  return true;
}

KeyboardEvdevHandler::KeyboardEvdevHandler(MessagePumpEpoll *pump,
                                           const char *devnode)
    : EvdevHandler(pump, devnode, BUF_LEN) {
  device_.is_repeatable = false;
}

void KeyboardEvdevHandler::ProcessKey(input_event *e) {
  if (e->value == 2 && !device_.is_repeatable)
    return;
  Notify(e->time, e->code, e->value, EVDEV_EVENT_KEY);
}

void KeyboardEvdevHandler::ProcessEvents(input_event *e) {
  switch (e->type) {
    case EV_KEY:
      ProcessKey(e);
      break;
    case EV_SYN:
      break;
  }
}

class MouseEvdevHandler : public EvdevHandler {
 public:
  MouseEvdevHandler(MessagePumpEpoll *pump, const char *devnode);
  virtual ~MouseEvdevHandler() {};

  virtual bool Configure() OVERRIDE;

 private:
  static const size_t BUF_LEN = 16;
  struct input_device {
    struct {
      uint32_t dx, dy;
      uint32_t sx, sy;
    } rel;

    int update_flags;
    bool is_repeatable;
  };

  virtual void ProcessEvents(input_event *e) OVERRIDE;
  void ProcessKey(input_event *e);
  void ProcessRelativeMotion(input_event *e);
  void FlushMotion(timeval time);
  input_device device_;

  DISALLOW_COPY_AND_ASSIGN(MouseEvdevHandler);
};

bool MouseEvdevHandler::Configure() {
  unsigned long ev_bits[NBITS(EV_CNT)];
  ioctl(fd_, EVIOCGBIT(0, sizeof(ev_bits)), ev_bits);
  if (TEST_BIT(ev_bits, EV_REP))
    device_.is_repeatable = true;
  return true;
}

MouseEvdevHandler::MouseEvdevHandler(MessagePumpEpoll *pump,
                                           const char *devnode)
    : EvdevHandler(pump, devnode, BUF_LEN) {
  device_.is_repeatable = false;
  device_.update_flags = 0;
  device_.rel.dx = 0;
  device_.rel.dy = 0;
  device_.rel.sx = 0;
  device_.rel.sy = 0;
}

void MouseEvdevHandler::ProcessKey(input_event *e) {
  if (e->value == 2 && !device_.is_repeatable)
    return;

  switch (e->code) {
  case BTN_TOUCH:
    // Treat BTN_TOUCH from devices that only have BTN_TOUCH as BTN_LEFT
    e->code = BTN_LEFT;
    // Intentional fallthrough!
  case BTN_LEFT:
  case BTN_RIGHT:
  case BTN_MIDDLE:
#if 0 // Not supported by Chromium
  case BTN_SIDE:
  case BTN_EXTRA:
  case BTN_FORWARD:
  case BTN_BACK:
  case BTN_TASK:
#endif
    Notify(e->time, e->code, e->value, EVDEV_EVENT_BUTTON);
    break;
  default:
    NOTREACHED();
    break;
  }
}

void MouseEvdevHandler::ProcessRelativeMotion(input_event *e) {
  // TODO(sheckylin) Make this configurable somehow.
  const int scroll_speed = 106;
  switch (e->code) {
  case REL_X:
    device_.rel.dx += e->value;
    device_.update_flags |= EVDEV_RELATIVE_MOTION;
    break;
  case REL_Y:
    device_.rel.dy += e->value;
    device_.update_flags |= EVDEV_RELATIVE_MOTION;
    break;
  case REL_WHEEL:
    device_.rel.sy += e->value * scroll_speed;
    device_.update_flags |= EVDEV_RELATIVE_SCROLL;
    break;
  case REL_HWHEEL:
    // The minus sign here is to match the common scroll experience.
    device_.rel.sx -= e->value * scroll_speed;
    device_.update_flags |= EVDEV_RELATIVE_SCROLL;
    break;
  }
}

void MouseEvdevHandler::FlushMotion(timeval time) {
  if (!device_.update_flags)
    return;

  if (device_.update_flags & EVDEV_RELATIVE_MOTION) {
    NotifyRelMotion(time, device_.rel.dx, device_.rel.dy);
    device_.rel.dx = 0;
    device_.rel.dy = 0;
  }
  if (device_.update_flags & EVDEV_RELATIVE_SCROLL) {
    NotifyScroll(time, device_.rel.sx, device_.rel.sy);
    device_.rel.sx = 0;
    device_.rel.sy = 0;
  }
  device_.update_flags = 0;
}

void MouseEvdevHandler::ProcessEvents(input_event *e) {
  switch (e->type) {
    case EV_REL:
      ProcessRelativeMotion(e);
      break;
    case EV_KEY:
      ProcessKey(e);
      break;
    case EV_SYN:
      FlushMotion(e->time);
      break;
  }
}

class TouchEvdevHandler : public EvdevHandler {
 public:
  TouchEvdevHandler(MessagePumpEpoll *pump, const char *devnode,
                    const size_t ev_buf_len);
  virtual ~TouchEvdevHandler() {}

  virtual bool Configure() OVERRIDE { return false; }

 protected:
  struct mt_slot {
    mt_slot(): update_flags(0), id(0), x(0), y(0), p(0), o(0), ma(0), mi(0) {}

    uint32_t update_flags;
    uint32_t id;
    uint32_t x, y;
    uint32_t p;
    uint32_t o, ma, mi;
  };

  virtual void ProcessEvents(input_event *e) OVERRIDE {}
  virtual void NotifyTouch(timeval time, const mt_slot &vals);

  DISALLOW_COPY_AND_ASSIGN(TouchEvdevHandler);
};

TouchEvdevHandler::TouchEvdevHandler(MessagePumpEpoll *pump,
                                     const char *devnode,
                                     const size_t ev_buf_len)
    : EvdevHandler(pump, devnode, ev_buf_len) {
}

void TouchEvdevHandler::NotifyTouch(timeval time, const mt_slot &vals) {
  EvdevEvent event;

  event.id = (uintptr_t) this;
  event.type = EVDEV_EVENT_TOUCH;
  event.time = time;
  event.code = vals.id;
  if (vals.update_flags & EVDEV_ABSOLUTE_DOWN)
    event.value = 1;
  else if (vals.update_flags & EVDEV_ABSOLUTE_UP)
    event.value = 0;
  else
    event.value = 2;
  event.x = vals.x;
  event.y = vals.y;
  event.touch.p = vals.p;
  event.touch.o = vals.o;
  event.touch.ma = vals.ma;
  event.touch.mi = vals.mi;
  pump_->DispatchEvent(&event);
}

class TouchpadEvdevHandler : public TouchEvdevHandler {
 public:
  TouchpadEvdevHandler(MessagePumpEpoll *pump, const char *devnode);
  virtual ~TouchpadEvdevHandler() {};

  virtual bool Configure() OVERRIDE;

 private:
  static const size_t BUF_LEN = 256;
  struct input_device {
    struct {
      int min_x, max_x, min_y, max_y;
      int old_x, old_y, reset_x, reset_y;
      uint32_t x, y;
    } abs;

    struct {
      uint32_t dx, dy;
    } rel;

    int update_flags;

    bool is_repeatable;
    bool is_touchpad, is_mt;
  };

  virtual void ProcessEvents(input_event *e) OVERRIDE;
  void ProcessKey(input_event *e);
  void ProcessAbsoluteMotionTouchpad(input_event *e);
  void FlushMotion(timeval time);
  input_device device_;

  DISALLOW_COPY_AND_ASSIGN(TouchpadEvdevHandler);
};

bool TouchpadEvdevHandler::Configure() {
  struct input_absinfo absinfo;
  unsigned long ev_bits[NBITS(EV_CNT)];
  unsigned long abs_bits[NBITS(ABS_CNT)];
  unsigned long key_bits[NBITS(KEY_CNT)];

  ioctl(fd_, EVIOCGBIT(0, sizeof(ev_bits)), ev_bits);
  if (TEST_BIT(ev_bits, EV_ABS)) {
    ioctl(fd_, EVIOCGBIT(EV_ABS, sizeof(abs_bits)), abs_bits);
    if (TEST_BIT(abs_bits, ABS_X)) {
      ioctl(fd_, EVIOCGABS(ABS_X), &absinfo);
      device_.abs.min_x = absinfo.minimum;
      device_.abs.max_x = absinfo.maximum;
    }
    if (TEST_BIT(abs_bits, ABS_Y)) {
      ioctl(fd_, EVIOCGABS(ABS_Y), &absinfo);
      device_.abs.min_y = absinfo.minimum;
      device_.abs.max_y = absinfo.maximum;
    }
    if (TEST_BIT(abs_bits, ABS_MT_SLOT))
      device_.is_mt = true;
  }
  if (TEST_BIT(ev_bits, EV_KEY)) {
    ioctl(fd_, EVIOCGBIT(EV_KEY, sizeof(key_bits)), key_bits);
    if (TEST_BIT(key_bits, BTN_TOOL_FINGER) &&
        !TEST_BIT(key_bits, BTN_TOOL_PEN))
      device_.is_touchpad = true;
    if (TEST_BIT(ev_bits, EV_REP)) {
      device_.is_repeatable = true;
    }
  }
  return true;
}

TouchpadEvdevHandler::TouchpadEvdevHandler(MessagePumpEpoll *pump,
                                           const char *devnode)
    : TouchEvdevHandler(pump, devnode, BUF_LEN) {
  device_.update_flags = 0;
  device_.is_repeatable = false;
  device_.is_touchpad = false;
  device_.is_mt = false;
  device_.rel.dx = 0;
  device_.rel.dy = 0;
}

void TouchpadEvdevHandler::ProcessKey(input_event *e) {
  if (e->value == 2 && !device_.is_repeatable)
    return;

  switch (e->code) {
  case BTN_TOOL_PEN:
  case BTN_TOOL_RUBBER:
  case BTN_TOOL_BRUSH:
  case BTN_TOOL_PENCIL:
  case BTN_TOOL_AIRBRUSH:
  case BTN_TOOL_FINGER:
  case BTN_TOOL_MOUSE:
  case BTN_TOOL_LENS:
    if (device_.is_touchpad) {
      device_.abs.reset_x = 1;
      device_.abs.reset_y = 1;
    }
    break;
  case BTN_TOUCH:
    // Multitouch touchscreen devices might not send individually
    // button events each time a new finger is down. So we don't
    // send notification for such devices and we solve the button
    // case emulating on compositor side.
    if (device_.is_mt)
      break;
    // Treat BTN_TOUCH from devices that only have BTN_TOUCH as BTN_LEFT
    e->code = BTN_LEFT;
    // Intentional fallthrough!
  case BTN_LEFT:
  case BTN_RIGHT:
  case BTN_MIDDLE:
    Notify(e->time, e->code, e->value, EVDEV_EVENT_BUTTON);
    break;
  default:
    NOTIMPLEMENTED();
    break;
  }
}

void TouchpadEvdevHandler::ProcessAbsoluteMotionTouchpad(
    input_event *e) {
  // TODO(msb) Make this configurable somehow.
  const int touchpad_speed = 700;

  switch (e->code) {
  case ABS_X:
    e->value -= device_.abs.min_x;
    if (device_.abs.reset_x)
      device_.abs.reset_x = 0;
    else
      device_.rel.dx =
          (e->value - device_.abs.old_x) * touchpad_speed /
          (device_.abs.max_x - device_.abs.min_x);
    device_.abs.old_x = e->value;
    device_.update_flags |= EVDEV_RELATIVE_MOTION;
    break;
  case ABS_Y:
    e->value -= device_.abs.min_y;
    if (device_.abs.reset_y)
      device_.abs.reset_y = 0;
    else
      device_.rel.dy =
          (e->value - device_.abs.old_y) * touchpad_speed /
          (device_.abs.max_y - device_.abs.min_y);
    device_.abs.old_y = e->value;
    device_.update_flags |= EVDEV_RELATIVE_MOTION;
    break;
  }
}

void TouchpadEvdevHandler::FlushMotion(timeval time) {
  if (!device_.update_flags)
    return;

  if (device_.update_flags & EVDEV_RELATIVE_MOTION) {
    NotifyRelMotion(time, device_.rel.dx, device_.rel.dy);
    device_.rel.dx = 0;
    device_.rel.dy = 0;
  }
  device_.update_flags = 0;
}

void TouchpadEvdevHandler::ProcessEvents(input_event *e) {
  switch (e->type) {
    case EV_ABS:
      ProcessAbsoluteMotionTouchpad(e);
      break;
    case EV_KEY:
      ProcessKey(e);
      break;
    case EV_SYN:
      FlushMotion(e->time);
      break;
  }
}

class TouchscreenEvdevHandler : public TouchEvdevHandler {
 public:
  TouchscreenEvdevHandler(MessagePumpEpoll *pump, const char *devnode);
  virtual ~TouchscreenEvdevHandler() {};

  virtual bool Configure() OVERRIDE;

 private:
  static const size_t BUF_LEN = 256;
  static const size_t MAX_SLOTS = 32;
  struct input_device {
    struct {
      mt_slot slots[MAX_SLOTS];
      std::vector<mt_slot *> updated_slots;
    } mt;

    struct {
      int id;
    } st;

    mt_slot *active_mt_slot;

    bool is_mt;
    bool is_oriented, is_circular;
  };

  virtual void ProcessEvents(input_event *e) OVERRIDE;
  void ProcessMultiTouch(input_event *e);
  void ProcessSimpleTouch(input_event *e);
  void ProcessKey(input_event *e);
  void FlushMotion(timeval time);
  input_device device_;

  DISALLOW_COPY_AND_ASSIGN(TouchscreenEvdevHandler);
};

bool TouchscreenEvdevHandler::Configure() {
  unsigned long ev_bits[NBITS(EV_CNT)];
  unsigned long abs_bits[NBITS(ABS_CNT)];

  ioctl(fd_, EVIOCGBIT(0, sizeof(ev_bits)), ev_bits);
  if (TEST_BIT(ev_bits, EV_ABS)) {
    ioctl(fd_, EVIOCGBIT(EV_ABS, sizeof(abs_bits)), abs_bits);
    if (TEST_BIT(abs_bits, ABS_MT_SLOT))
      device_.is_mt = true;
    if (TEST_BIT(abs_bits, ABS_MT_ORIENTATION))
      device_.is_oriented = true;
    if (TEST_BIT(abs_bits, ABS_MT_TOUCH_MINOR))
      device_.is_circular = false;
  }
  return true;
}

TouchscreenEvdevHandler::TouchscreenEvdevHandler(MessagePumpEpoll *pump,
                                           const char *devnode)
    : TouchEvdevHandler(pump, devnode, BUF_LEN) {
  device_.is_mt = false;
  device_.is_oriented = false;
  device_.is_circular = true;
  device_.active_mt_slot = device_.mt.slots;
  device_.st.id = 0;
}

void TouchscreenEvdevHandler::ProcessKey(input_event *e) {
  switch (e->code) {
  case BTN_TOUCH:
    // Multitouch touchscreen devices might not send individually
    // button events each time a new finger is down. So we don't
    // send notification for such devices and we solve the button
    // case emulating on compositor side.
    if (device_.is_mt)
      break;
    if (e->value) {
      device_.active_mt_slot->id = device_.st.id++;
      device_.active_mt_slot->update_flags |= EVDEV_ABSOLUTE_DOWN;
    } else
      device_.active_mt_slot->update_flags |= EVDEV_ABSOLUTE_UP;
    if (device_.mt.updated_slots.empty())
      device_.mt.updated_slots.push_back(device_.active_mt_slot);
    break;
  default:
    NOTIMPLEMENTED();
    break;
  }
}

void TouchscreenEvdevHandler::ProcessSimpleTouch(input_event *e) {
  // We will treat single-touch touchscreens as single-slot MT touchscreens.
  switch (e->code) {
  case ABS_X:
    device_.active_mt_slot->x = e->value;
    break;
  case ABS_Y:
    device_.active_mt_slot->y = e->value;
    break;
  case ABS_PRESSURE:
    device_.active_mt_slot->ma = e->value;
    device_.active_mt_slot->mi = e->value;
    device_.active_mt_slot->p = e->value;
    break;
  }

  if (device_.mt.updated_slots.empty())
    device_.mt.updated_slots.push_back(device_.active_mt_slot);
}

void TouchscreenEvdevHandler::ProcessMultiTouch(input_event *e) {
  switch (e->code) {
  case ABS_MT_SLOT:
    device_.active_mt_slot = device_.mt.slots + e->value;
    break;
  case ABS_MT_TRACKING_ID:
    if (e->value >= 0) {
      device_.active_mt_slot->id = e->value;
      device_.active_mt_slot->update_flags |= EVDEV_ABSOLUTE_DOWN;
    } else
      device_.active_mt_slot->update_flags |= EVDEV_ABSOLUTE_UP;
    break;
  case ABS_MT_POSITION_X:
    device_.active_mt_slot->x = e->value;
    break;
  case ABS_MT_POSITION_Y:
    device_.active_mt_slot->y = e->value;
    break;
  case ABS_MT_PRESSURE:
    device_.active_mt_slot->p = e->value;
    break;
  case ABS_MT_TOUCH_MAJOR:
    device_.active_mt_slot->ma = e->value;
    if (device_.is_circular)
      device_.active_mt_slot->mi = e->value;
    break;
  case ABS_MT_TOUCH_MINOR:
    device_.active_mt_slot->mi = e->value;
    break;
  case ABS_MT_ORIENTATION:
    device_.active_mt_slot->o = e->value;
    break;
  }

  // The logic here assumes will be at least one MT event for every non-MT
  // event on an MT device. Otherwise it might send ghost updates.
  if (device_.mt.updated_slots.empty() ||
      device_.mt.updated_slots.back() != device_.active_mt_slot)
    device_.mt.updated_slots.push_back(device_.active_mt_slot);
}

void TouchscreenEvdevHandler::FlushMotion(timeval time) {
  while (!device_.mt.updated_slots.empty()) {
    mt_slot *current_slot = device_.mt.updated_slots.back();
    device_.mt.updated_slots.pop_back();
    NotifyTouch(time, *current_slot);
    current_slot->update_flags = 0;
  }
}

void TouchscreenEvdevHandler::ProcessEvents(input_event *e) {
  switch (e->type) {
    case EV_ABS:
      device_.is_mt ? ProcessMultiTouch(e) : ProcessSimpleTouch(e);
      break;
    case EV_KEY:
      ProcessKey(e);
      break;
    case EV_SYN:
      FlushMotion(e->time);
      break;
  }
}

class CommonEvdevHandler : public EvdevHandler {
 public:
  CommonEvdevHandler(MessagePumpEpoll *pump, const char *devnode);
  virtual ~CommonEvdevHandler() {};

  virtual bool Configure() OVERRIDE;

 private:
  static const size_t BUF_LEN = 8;
  struct input_device {
    struct {
      int min_x, max_x, min_y, max_y;
    } abs;

    bool is_repeatable;
    bool is_touchpad, is_mt;
    bool is_oriented, is_circular;
  };

  virtual void ProcessEvents(input_event *e) OVERRIDE;
  input_device device_;

  DISALLOW_COPY_AND_ASSIGN(CommonEvdevHandler);
};

bool CommonEvdevHandler::Configure() {
  struct input_absinfo absinfo;
  unsigned long ev_bits[NBITS(EV_CNT)];
  unsigned long abs_bits[NBITS(ABS_CNT)];
  unsigned long key_bits[NBITS(KEY_CNT)];
  bool has_key = false, has_abs = false;

  ioctl(fd_, EVIOCGBIT(0, sizeof(ev_bits)), ev_bits);
  if (TEST_BIT(ev_bits, EV_ABS)) {
    has_abs = true;
    ioctl(fd_, EVIOCGBIT(EV_ABS, sizeof(abs_bits)), abs_bits);
    if (TEST_BIT(abs_bits, ABS_X)) {
      ioctl(fd_, EVIOCGABS(ABS_X), &absinfo);
      device_.abs.min_x = absinfo.minimum;
      device_.abs.max_x = absinfo.maximum;
    }
    if (TEST_BIT(abs_bits, ABS_Y)) {
      ioctl(fd_, EVIOCGABS(ABS_Y), &absinfo);
      device_.abs.min_y = absinfo.minimum;
      device_.abs.max_y = absinfo.maximum;
    }
    if (TEST_BIT(abs_bits, ABS_MT_SLOT))
      device_.is_mt = true;
    if (TEST_BIT(abs_bits, ABS_MT_ORIENTATION))
      device_.is_oriented = true;
    if (TEST_BIT(abs_bits, ABS_MT_TOUCH_MINOR))
      device_.is_circular = false;
  }
  if (TEST_BIT(ev_bits, EV_KEY)) {
    has_key = true;
    ioctl(fd_, EVIOCGBIT(EV_KEY, sizeof(key_bits)), key_bits);
    if (TEST_BIT(key_bits, BTN_TOOL_FINGER) &&
        !TEST_BIT(key_bits, BTN_TOOL_PEN))
      device_.is_touchpad = true;
    if (TEST_BIT(ev_bits, EV_REP)) {
      device_.is_repeatable = true;
    }
  }

  // This rule tries to catch accelerometer devices and opt out. We may
  // want to adjust the protocol later adding a proper event for dealing
  // with accelerometers and implement here accordingly
  if (has_abs && !has_key)
    return false;
  return true;
}

CommonEvdevHandler::CommonEvdevHandler(MessagePumpEpoll *pump,
                                       const char *devnode)
    : EvdevHandler(pump, devnode, BUF_LEN) {
  device_.is_repeatable = false;
  device_.is_touchpad = false;
  device_.is_mt = false;
  device_.is_oriented = false;
  device_.is_circular = true;
}

void CommonEvdevHandler::ProcessEvents(input_event *e) {
  switch (e->type) {
    case EV_REL:
    case EV_ABS:
    case EV_KEY:
    case EV_SYN:
      Notify(e->time, e->code, e->value, EVDEV_EVENT_UNKNOWN);
      break;
    default:
      NOTIMPLEMENTED();
      break;
  }
}

class UdevHandler : public MessagePumpEpollHandler {
 public:
  UdevHandler(MessagePumpEpoll *pump);
  virtual ~UdevHandler();

  virtual void Process() OVERRIDE;

 private:
  void AddDevice(struct udev_device *device);
  void RemoveDevice(const char *devname);
  void AddDevices(udev *udev);
  void RemoveDevices();
  udev_monitor *udev_monitor_;
  std::map<std::string, EvdevHandler *> input_list_;

  DISALLOW_COPY_AND_ASSIGN(UdevHandler);
};

UdevHandler::UdevHandler(MessagePumpEpoll *pump)
    : MessagePumpEpollHandler(pump) {
  int ret;

  udev *udev = udev_new();
  udev_monitor_ = udev_monitor_new_from_netlink(udev, "udev");
  udev_monitor_filter_add_match_subsystem_devtype(udev_monitor_, "input", NULL);

  ret = udev_monitor_enable_receiving(udev_monitor_);
  DCHECK_EQ(ret, 0);
  (void)ret; // Prevent warning in release mode.

  fd_ = udev_monitor_get_fd(udev_monitor_);

  AddDevices(udev);
}

UdevHandler::~UdevHandler() {
  RemoveDevices();
}

void UdevHandler::AddDevice(struct udev_device *device) {
  const char* devnode = udev_device_get_devnode(device);

  // Identify the device type.
  EvdevHandler *handler;
  if (udev_device_get_property_value(device, "ID_INPUT_TOUCHSCREEN"))
    handler = new TouchscreenEvdevHandler(pump_, devnode);
  else if (udev_device_get_property_value(device, "ID_INPUT_TOUCHPAD"))
    handler = new TouchpadEvdevHandler(pump_, devnode);
  else if (udev_device_get_property_value(device, "ID_INPUT_KEYBOARD"))
    handler = new KeyboardEvdevHandler(pump_, devnode);
  else if (udev_device_get_property_value(device, "ID_INPUT_MOUSE"))
    handler = new MouseEvdevHandler(pump_, devnode);
  else
    handler = new CommonEvdevHandler(pump_, devnode);

  if (!handler->Configure()) {
    delete handler;
    return;
  }

  input_list_[devnode] = handler;
  pump_->AddHandler(handler);
}

void UdevHandler::RemoveDevice(const char *devnode) {
  EvdevHandler *handler = input_list_[devnode];

  pump_->RemoveHandler(handler);
  input_list_.erase(handler->devnode());

  delete handler;
}

void UdevHandler::AddDevices(udev *udev) {
  struct udev_enumerate *e;
  struct udev_list_entry *entry;

  e = udev_enumerate_new(udev);
  udev_enumerate_add_match_subsystem(e, "input");
  udev_enumerate_scan_devices(e);
  udev_list_entry_foreach(entry, udev_enumerate_get_list_entry(e)) {
    const char *path = udev_list_entry_get_name(entry);
    struct udev_device *device = udev_device_new_from_syspath(udev, path);

    if (strncmp("event", udev_device_get_sysname(device), 5))
      continue;

    AddDevice(device);

    udev_device_unref(device);
  }
  udev_enumerate_unref(e);
}

void UdevHandler::RemoveDevices() {
  std::map<std::string, EvdevHandler *>::iterator it;

  for (it = input_list_.begin(); it != input_list_.end(); )
    RemoveDevice((it++)->first.c_str());
}

void UdevHandler::Process() {
  struct udev_device *udev_device;
  const char *action;

  udev_device = udev_monitor_receive_device(udev_monitor_);
  if (!udev_device)
    return;

  action = udev_device_get_action(udev_device);
  if (!action)
    return;

  if (strncmp("event", udev_device_get_sysname(udev_device), 5))
    return;

  if (!strcmp(action, "add"))
    AddDevice(udev_device);
  else if (!strcmp(action, "remove"))
    RemoveDevice(udev_device_get_devnode(udev_device));

  udev_device_unref(udev_device);
}

}  // namespace

MessagePumpEvdev::MessagePumpEvdev()
    : udev_handler_(new UdevHandler(this)) {
  AddHandler(udev_handler_);
}

MessagePumpEvdev::~MessagePumpEvdev() {
  delete(udev_handler_);
}

}  // namespace base
