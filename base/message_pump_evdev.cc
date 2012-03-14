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

#include "base/message_pump_evdev.h"

#include <stdint.h>
#include <string.h>
#include <libudev.h>
#include <linux/input.h>
#include <unistd.h>
#include <fcntl.h>

#include <map>
#include <string>

#define MAX_SLOTS 16

// event type flags
#define EVDEV_ABSOLUTE_MOTION		(1 << 0)
#define EVDEV_ABSOLUTE_MT_DOWN		(1 << 1)
#define EVDEV_ABSOLUTE_MT_MOTION	(1 << 2)
#define EVDEV_ABSOLUTE_MT_UP		(1 << 3)
#define EVDEV_RELATIVE_MOTION		(1 << 4)

// TODO(msb) fix this
# define WL_INPUT_DEVICE_TOUCH_DOWN 0
# define WL_INPUT_DEVICE_TOUCH_UP 0
# define WL_INPUT_DEVICE_TOUCH_MOTION 0

namespace base {

namespace {

struct evdev_input_device {
  struct {
    int min_x, max_x, min_y, max_y;
    int old_x, old_y, reset_x, reset_y;
    uint32_t x, y;
  } abs;

  struct {
    int slot;
    uint32_t x[MAX_SLOTS];
    uint32_t y[MAX_SLOTS];
  } mt;

  struct {
    uint32_t dx, dy;
  } rel;

  int type;

  bool is_touchpad, is_mt;
};

class EvdevHandler : public MessagePumpEpollHandler {
 public:
  EvdevHandler(MessagePumpEpoll *pump, const char *devnode);
  virtual ~EvdevHandler();

  virtual void Process() OVERRIDE;
  bool Configure();

  char *devnode() { return devnode_; };

 private:
  void ProcessKey(input_event *e);
  void ProcessTouch(input_event *e);
  void ProcessAbsoluteMotion(input_event *e);
  void ProcessAbsoluteMotionTouchpad(input_event *e);
  void ProcessRelativeMotion(input_event *e);
  void ProcessAbsolute(input_event *e);
  bool IsMotionEvent(input_event *e);
  void FlushMotion(timeval time);
  void NotifyButton(timeval time, uint16_t code, uint32_t value);
  void NotifyKey(timeval time, uint16_t code, uint32_t value);
  void NotifyRelMotion(timeval time, uint32_t dx, uint32_t dy);
  void NotifyAbsMotion(timeval time, uint32_t x, uint32_t y);
  void NotifyTouch(timeval time, int touch_id,
                   uint32_t x, uint32_t y, int touch_type);

  char *devnode_;
  struct evdev_input_device device_;

  DISALLOW_COPY_AND_ASSIGN(EvdevHandler);
};

// copied from udev/extras/input_id/input_id.c
// we must use this kernel-compatible implementation
#define BITS_PER_LONG (sizeof(unsigned long) * 8)
#define NBITS(x) ((((x)-1)/BITS_PER_LONG)+1)
#define OFF(x)  ((x)%BITS_PER_LONG)
#define BIT(x)  (1UL<<OFF(x))
#define LONG(x) ((x)/BITS_PER_LONG)
#define TEST_BIT(array, bit)    ((array[LONG(bit)] >> OFF(bit)) & 1)
// end copied

bool EvdevHandler::Configure() {
  struct input_absinfo absinfo;
  unsigned long ev_bits[NBITS(EV_MAX)];
  unsigned long abs_bits[NBITS(ABS_MAX)];
  unsigned long key_bits[NBITS(KEY_MAX)];
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
    if (TEST_BIT(abs_bits, ABS_MT_SLOT)) {
      device_.is_mt = true;
      device_.mt.slot = 0;
    }
  }
  if (TEST_BIT(ev_bits, EV_KEY)) {
    has_key = true;
    ioctl(fd_, EVIOCGBIT(EV_KEY, sizeof(key_bits)), key_bits);
    if (TEST_BIT(key_bits, BTN_TOOL_FINGER) &&
	!TEST_BIT(key_bits, BTN_TOOL_PEN))
      device_.is_touchpad = true;
  }

  // This rule tries to catch accelerometer devices and opt out. We may
  // want to adjust the protocol later adding a proper event for dealing
  // with accelerometers and implement here accordingly
  if (has_abs && !has_key)
    return false;

  return true;
}

EvdevHandler::EvdevHandler(MessagePumpEpoll *pump, const char *devnode)
    : MessagePumpEpollHandler(pump) {
  fd_ = open(devnode, O_RDONLY);
  DCHECK_GE(fd_, 0);
  device_.is_touchpad = false;
  device_.is_mt = false;
  devnode_ = strdup(devnode);
  device_.mt.slot = -1;
  device_.rel.dx = 0;
  device_.rel.dy = 0;
}

EvdevHandler::~EvdevHandler() {
  free(devnode_);
}

void EvdevHandler::ProcessKey(input_event *e) {
  if (e->value == 2)
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
#if 0 // Not supported by Chromium
  case BTN_SIDE:
  case BTN_EXTRA:
  case BTN_FORWARD:
  case BTN_BACK:
  case BTN_TASK:
#endif
    NotifyButton(e->time, e->code, e->value);
    break;
  default:
    NotifyKey(e->time, e->code, e->value);
    break;
  }
}

void EvdevHandler::ProcessTouch(input_event *e) {
  switch (e->code) {
  case ABS_MT_SLOT:
    device_.mt.slot = e->value;
    break;
  case ABS_MT_TRACKING_ID:
    if (e->value >= 0)
      device_.type |= EVDEV_ABSOLUTE_MT_DOWN;
    else
      device_.type |= EVDEV_ABSOLUTE_MT_UP;
    break;
  case ABS_MT_POSITION_X:
    device_.mt.x[device_.mt.slot] = e->value;
    device_.type |= EVDEV_ABSOLUTE_MT_MOTION;
    break;
  case ABS_MT_POSITION_Y:
    device_.mt.y[device_.mt.slot] = e->value;
    device_.type |= EVDEV_ABSOLUTE_MT_MOTION;
    break;
  }
}

void EvdevHandler::ProcessAbsoluteMotion(input_event *e) {
  switch (e->code) {
  case ABS_X:
    device_.abs.x = e->value;
    device_.type |= EVDEV_ABSOLUTE_MOTION;
    break;
  case ABS_Y:
    device_.abs.y = e->value;
    device_.type |= EVDEV_ABSOLUTE_MOTION;
    break;
  }
}

void EvdevHandler::ProcessAbsoluteMotionTouchpad(
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
    device_.type |= EVDEV_RELATIVE_MOTION;
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
    device_.type |= EVDEV_RELATIVE_MOTION;
    break;
  }
}

void EvdevHandler::ProcessRelativeMotion(input_event *e) {
	switch (e->code) {
	case REL_X:
		device_.rel.dx += e->value;
		device_.type |= EVDEV_RELATIVE_MOTION;
		break;
	case REL_Y:
		device_.rel.dy += e->value;
		device_.type |= EVDEV_RELATIVE_MOTION;
		break;
	}
}

void EvdevHandler::ProcessAbsolute(input_event *e)
{
  if (device_.is_touchpad)
    ProcessAbsoluteMotion(e);
  else if (device_.is_mt)
    ProcessTouch(e);
  else
    ProcessAbsoluteMotion(e);
}

bool EvdevHandler::IsMotionEvent(input_event *e) {
  switch (e->type) {
  case EV_REL:
    switch (e->code) {
    case REL_X:
    case REL_Y:
      return true;
    }
  case EV_ABS:
    switch (e->code) {
    case ABS_X:
    case ABS_Y:
    case ABS_MT_POSITION_X:
    case ABS_MT_POSITION_Y:
      return true;
    }
  }
  return false;
}

void EvdevHandler::FlushMotion(timeval time) {
  if (!device_.type)
    return;

  if (device_.type & EVDEV_RELATIVE_MOTION) {
    NotifyRelMotion(time, device_.rel.dx, device_.rel.dy);
    device_.type &= ~EVDEV_RELATIVE_MOTION;
    device_.rel.dx = 0;
    device_.rel.dy = 0;
  }
  if (device_.type & EVDEV_ABSOLUTE_MT_DOWN) {
    NotifyTouch(time,
		device_.mt.slot,
		device_.mt.x[device_.mt.slot],
		device_.mt.y[device_.mt.slot],
		WL_INPUT_DEVICE_TOUCH_DOWN);
    device_.type &= ~EVDEV_ABSOLUTE_MT_DOWN;
    device_.type &= ~EVDEV_ABSOLUTE_MT_MOTION;
  }
  if (device_.type & EVDEV_ABSOLUTE_MT_MOTION) {
    NotifyTouch(time,
		device_.mt.slot,
		device_.mt.x[device_.mt.slot],
		device_.mt.y[device_.mt.slot],
		WL_INPUT_DEVICE_TOUCH_MOTION);
    device_.type &= ~EVDEV_ABSOLUTE_MT_DOWN;
    device_.type &= ~EVDEV_ABSOLUTE_MT_MOTION;
  }
  if (device_.type & EVDEV_ABSOLUTE_MT_UP) {
    NotifyTouch(time, device_.mt.slot, 0, 0,
		WL_INPUT_DEVICE_TOUCH_UP);
    device_.type &= ~EVDEV_ABSOLUTE_MT_UP;
  }
  if (device_.type & EVDEV_ABSOLUTE_MOTION) {
    NotifyAbsMotion(time, device_.abs.x, device_.abs.y);
    device_.type &= ~EVDEV_ABSOLUTE_MOTION;
  }
}

void EvdevHandler::Process() {
  input_event ev[8], *e, *end;
  int len;

  len = read(fd(), &ev, sizeof ev);
  if (len < 0 || len % sizeof e[0] != 0)
    return;

  device_.type = 0;

  end = ev + (len / sizeof e[0]);
  for (e = ev; e < end; ++e) {
    /* we try to minimize the amount of notifications to be
     * forwarded to the compositor, so we accumulate motion
     * events and send as a bunch */
    if (!IsMotionEvent(e))
      FlushMotion(e->time);
    switch (e->type) {
    case EV_REL:
      ProcessRelativeMotion(e);
      break;
    case EV_ABS:
      ProcessAbsolute(e);
      break;
    case EV_KEY:
      ProcessKey(e);
      break;
    }
  }
  FlushMotion((--e)->time);
}

void EvdevHandler::NotifyButton(timeval time, uint16_t code, uint32_t value) {
  EvdevEvent event;

  event.type = EVDEV_EVENT_BUTTON;
  event.time = time;
  event.code = code;
  event.value = value;
  pump_->DispatchEvent(&event);
}
void EvdevHandler::NotifyKey(timeval time, uint16_t code, uint32_t value) {
  EvdevEvent event;

  event.type = EVDEV_EVENT_KEY;
  event.time = time;
  event.code = code;
  event.value = value;
  pump_->DispatchEvent(&event);
}
void EvdevHandler::NotifyRelMotion(timeval time, uint32_t dx, uint32_t dy) {
  EvdevEvent event;

  event.type = EVDEV_EVENT_RELATIVE_MOTION;
  event.time = time;
  event.motion.x = dx;
  event.motion.y = dy;
  pump_->DispatchEvent(&event);
}
void EvdevHandler::NotifyAbsMotion(timeval time, uint32_t x, uint32_t y) {
  NOTIMPLEMENTED();
}
void EvdevHandler::NotifyTouch(timeval time, int touch_id,
                               uint32_t x, uint32_t y, int touch_type) {
  NOTIMPLEMENTED();
}


class UdevHandler : public MessagePumpEpollHandler {
 public:
  UdevHandler(MessagePumpEpoll *pump);
  virtual ~UdevHandler();

  virtual void Process() OVERRIDE;

 private:
  void AddDevice(const char *devname);
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

void UdevHandler::AddDevice(const char *devnode) {
  EvdevHandler *handler = new EvdevHandler(pump_, devnode);

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

    AddDevice(udev_device_get_devnode(device));

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
    AddDevice(udev_device_get_devnode(udev_device));
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
