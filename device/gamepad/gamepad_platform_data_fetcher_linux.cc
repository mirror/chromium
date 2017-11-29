// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/gamepad/gamepad_platform_data_fetcher_linux.h"

#include <fcntl.h>
#include <limits.h>
#include <linux/input.h>
#include <linux/joystick.h>
#include <stddef.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <algorithm>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/posix/eintr_wrapper.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "base/trace_event/trace_event.h"
#include "device/udev_linux/scoped_udev.h"
#include "device/udev_linux/udev_linux.h"

namespace {

const char kInputSubsystem[] = "input";
const char kUsbSubsystem[] = "usb";
const char kUsbDeviceType[] = "usb_device";
const float kMaxLinuxAxisValue = 32767.0;
const int kInvalidEffectId = -1;
const uint16_t kRumbleMagnitudeMax = 0xffff;

#define LONG_BITS (CHAR_BIT * sizeof(long))
#define BITS_TO_LONGS(x) (((x) + LONG_BITS - 1) / LONG_BITS)

static inline bool test_bit(int bit, const unsigned long* data) {
  return data[bit / LONG_BITS] & (1UL << (bit % LONG_BITS));
}

void CloseFileDescriptorIfValid(int fd) {
  if (fd >= 0)
    close(fd);
}

int DeviceIndexFromDevicePath(const std::string& path,
                              const std::string& prefix) {
  if (!base::StartsWith(path, prefix, base::CompareCase::SENSITIVE))
    return -1;

  int index = -1;
  base::StringPiece index_str(&path.c_str()[prefix.length()],
                              path.length() - prefix.length());
  if (!base::StringToInt(index_str, &index))
    return -1;

  return index;
}

bool IsUdevGamepad(udev_device* dev, device::UdevGamepad* pad_info) {
  using DeviceRootPair = std::pair<device::UdevGamepadType, const char*>;
  static const std::vector<DeviceRootPair> device_roots = {
      {device::UdevGamepadType::EVDEV, "/dev/input/event"},
      {device::UdevGamepadType::JOYDEV, "/dev/input/js"},
  };

  if (!dev)
    return false;

  if (!device::udev_device_get_property_value(dev, "ID_INPUT_JOYSTICK"))
    return false;

  const char* node_path = device::udev_device_get_devnode(dev);
  if (!node_path)
    return false;

  // The device pointed to by parent_dev contains information about the logical
  // joystick device. We can compare syspaths to determine when a joydev device
  // and an evdev device refer to the same logical joystick.
  udev_device* parent_dev =
      device::udev_device_get_parent_with_subsystem_devtype(
          dev, kInputSubsystem, nullptr);
  const char* parent_syspath =
      parent_dev ? device::udev_device_get_syspath(parent_dev) : "";

  for (const auto& entry : device_roots) {
    device::UdevGamepadType node_type = entry.first;
    const char* prefix = entry.second;
    int index_value = DeviceIndexFromDevicePath(node_path, prefix);

    if (index_value < 0)
      continue;

    if (pad_info) {
      pad_info->type = node_type;
      pad_info->index = index_value;
      pad_info->path = node_path;
      pad_info->parent_syspath = parent_syspath;
    }

    return true;
  }

  return false;
}

bool HasForceFeedbackCapability(int fd) {
  unsigned long evbit[BITS_TO_LONGS(EV_MAX)];
  unsigned long ffbit[BITS_TO_LONGS(FF_MAX)];

  if (HANDLE_EINTR(ioctl(fd, EVIOCGBIT(0, EV_MAX), evbit)) < 0 ||
      HANDLE_EINTR(ioctl(fd, EVIOCGBIT(EV_FF, FF_MAX), ffbit)) < 0) {
    return false;
  }

  if (!test_bit(EV_FF, evbit)) {
    return false;
  }

  return test_bit(FF_RUMBLE, ffbit);
}

int StoreRumbleEffect(int fd,
                      int effect_id,
                      uint16_t duration,
                      uint16_t start_delay,
                      uint16_t strong_magnitude,
                      uint16_t weak_magnitude) {
  struct ff_effect effect;
  memset(&effect, 0, sizeof(effect));
  effect.type = FF_RUMBLE;
  effect.id = effect_id;
  effect.replay.length = duration;
  effect.replay.delay = start_delay;
  effect.u.rumble.strong_magnitude = strong_magnitude;
  effect.u.rumble.weak_magnitude = weak_magnitude;

  if (HANDLE_EINTR(ioctl(fd, EVIOCSFF, (const void*)&effect)) < 0)
    return kInvalidEffectId;
  return effect.id;
}

bool StartOrStopEffect(int fd, int effect_id, bool do_start) {
  struct input_event start_stop;
  memset(&start_stop, 0, sizeof(start_stop));
  start_stop.type = EV_FF;
  start_stop.code = effect_id;
  start_stop.value = do_start ? 1 : 0;
  ssize_t nbytes =
      HANDLE_EINTR(write(fd, (const void*)&start_stop, sizeof(start_stop)));
  return nbytes == sizeof(start_stop);
}

}  // namespace

namespace device {

GamepadPlatformDataFetcherLinux::GamepadPlatformDataFetcherLinux() {
  std::fill(joydev_to_evdev_, joydev_to_evdev_ + Gamepads::kItemsLengthCap, -1);
  std::fill(sequence_ids_, sequence_ids_ + Gamepads::kItemsLengthCap, 0);
}

GamepadPlatformDataFetcherLinux::~GamepadPlatformDataFetcherLinux() {
  for (size_t i = 0; i < Gamepads::kItemsLengthCap; ++i) {
    if (js_devices_[i])
      CloseFileDescriptorIfValid(js_devices_[i]->fd);
  }
  for (const auto& entry : ev_devices_) {
    const EvDeviceInfo& ev_info = *entry.second.get();
    CloseFileDescriptorIfValid(ev_info.fd);
  }
}

GamepadSource GamepadPlatformDataFetcherLinux::source() {
  return Factory::static_source();
}

void GamepadPlatformDataFetcherLinux::OnAddedToProvider() {
  std::vector<UdevLinux::UdevMonitorFilter> filters;
  filters.push_back(UdevLinux::UdevMonitorFilter(kInputSubsystem, nullptr));
  udev_.reset(new UdevLinux(
      filters, base::Bind(&GamepadPlatformDataFetcherLinux::RefreshDevice,
                          base::Unretained(this))));

  EnumerateDevices();
}

void GamepadPlatformDataFetcherLinux::GetGamepadData(bool) {
  TRACE_EVENT0("GAMEPAD", "GetGamepadData");

  // Update our internal state.
  for (size_t i = 0; i < Gamepads::kItemsLengthCap; ++i) {
    const auto& js_info = js_devices_[i];
    if (js_info && js_info->fd >= 0) {
      ReadDeviceData(i);
    }
  }
}

// Used during enumeration, and monitor notifications.
void GamepadPlatformDataFetcherLinux::RefreshDevice(udev_device* dev) {
  UdevGamepad pad_info;
  if (IsUdevGamepad(dev, &pad_info)) {
    if (pad_info.type == UdevGamepadType::JOYDEV)
      RefreshJoydevDevice(dev, pad_info);
    else if (pad_info.type == UdevGamepadType::EVDEV)
      RefreshEvdevDevice(dev, pad_info);
  }
}

void GamepadPlatformDataFetcherLinux::RefreshJoydevDevice(
    udev_device* dev,
    const UdevGamepad& pad_info) {
  const int js_index = pad_info.index;
  if (js_index < 0 || js_index >= (int)Gamepads::kItemsLengthCap)
    return;

  JsDeviceInfo& js_info = GetOrCreateJsDeviceInfo(js_index);
  CloseFileDescriptorIfValid(js_info.fd);

  int ev_match = joydev_to_evdev_[js_index];

  if (pad_info.parent_syspath.empty()) {
    joydev_to_evdev_[js_index] = -1;
    js_devices_[js_index].reset();
    return;
  }

  // The device pointed to by dev contains information about the logical
  // joystick device. In order to get the information about the physical
  // hardware, get the parent device that is also in the "input" subsystem.
  // This function should just walk up the tree one level.
  udev_device* parent_dev =
      device::udev_device_get_parent_with_subsystem_devtype(
          dev, kInputSubsystem, nullptr);
  if (!parent_dev) {
    joydev_to_evdev_[js_index] = -1;
    js_devices_[js_index].reset();
    return;
  }

  if (js_info.parent_syspath != pad_info.parent_syspath)
    ev_match = -1;

  if (ev_match < 0) {
    for (const auto& entry : ev_devices_) {
      const EvDeviceInfo& ev_info = *entry.second.get();
      if (ev_info.parent_syspath == pad_info.parent_syspath) {
        ev_match = entry.first;
        break;
      }
    }
  }

  // If the device cannot be opened, the joystick has been disconnected.
  int device_fd = open(pad_info.path.c_str(), O_RDONLY | O_NONBLOCK);
  if (device_fd < 0) {
    joydev_to_evdev_[js_index] = -1;
    js_devices_[js_index].reset();
    return;
  }

  PadState* state = GetPadState(js_index);
  if (!state) {
    // No slot available for device, don't use.
    CloseFileDescriptorIfValid(device_fd);
    joydev_to_evdev_[js_index] = -1;
    js_devices_[js_index].reset();
    return;
  }

  js_info.fd = device_fd;
  js_info.parent_syspath = pad_info.parent_syspath;

  bool has_ff_capability = false;
  if (ev_match >= 0) {
    joydev_to_evdev_[js_index] = ev_match;

    // Check the evdev device for force feedback capability.
    EvDeviceInfo* ev_info = EvDeviceInfoFromJsIndex(js_index);
    if (ev_info)
      has_ff_capability = ev_info->has_ff_capability;
  }

  Gamepad& pad = state->data;
  GamepadStandardMappingFunction& mapper = state->mapper;

  const char* vendor_id =
      udev_device_get_sysattr_value(parent_dev, "id/vendor");
  const char* product_id =
      udev_device_get_sysattr_value(parent_dev, "id/product");
  const char* version_number =
      udev_device_get_sysattr_value(parent_dev, "id/version");
  mapper =
      GetGamepadStandardMappingFunction(vendor_id, product_id, version_number);

  // Driver returns utf-8 strings here, so combine in utf-8 first and
  // convert to WebUChar later once we've picked an id string.
  const char* name = udev_device_get_sysattr_value(parent_dev, "name");
  std::string name_string(name ? name : "");

  // In many cases the information the input subsystem contains isn't
  // as good as the information that the device bus has, walk up further
  // to the subsystem/device type "usb"/"usb_device" and if this device
  // has the same vendor/product id, prefer the description from that.
  struct udev_device* usb_dev = udev_device_get_parent_with_subsystem_devtype(
      parent_dev, kUsbSubsystem, kUsbDeviceType);
  if (usb_dev) {
    const char* usb_vendor_id =
        udev_device_get_sysattr_value(usb_dev, "idVendor");
    const char* usb_product_id =
        udev_device_get_sysattr_value(usb_dev, "idProduct");

    if (vendor_id && product_id && strcmp(vendor_id, usb_vendor_id) == 0 &&
        strcmp(product_id, usb_product_id) == 0) {
      const char* manufacturer =
          udev_device_get_sysattr_value(usb_dev, "manufacturer");
      const char* product = udev_device_get_sysattr_value(usb_dev, "product");

      // Replace the previous name string with one containing the better
      // information, again driver returns utf-8 strings here so combine
      // in utf-8 for conversion to WebUChar below.
      name_string = base::StringPrintf("%s %s", manufacturer, product);
    }
  }

  // Append the vendor and product information then convert the utf-8
  // id string to WebUChar.
  std::string id =
      name_string + base::StringPrintf(" (%sVendor: %s Product: %s)",
                                       mapper ? "STANDARD GAMEPAD " : "",
                                       vendor_id, product_id);
  base::TruncateUTF8ToByteSize(id, Gamepad::kIdLengthCap - 1, &id);
  base::string16 tmp16 = base::UTF8ToUTF16(id);
  memset(pad.id, 0, sizeof(pad.id));
  tmp16.copy(pad.id, arraysize(pad.id) - 1);

  if (mapper) {
    std::string mapping = "standard";
    base::TruncateUTF8ToByteSize(mapping, Gamepad::kMappingLengthCap - 1,
                                 &mapping);
    tmp16 = base::UTF8ToUTF16(mapping);
    memset(pad.mapping, 0, sizeof(pad.mapping));
    tmp16.copy(pad.mapping, arraysize(pad.mapping) - 1);
  } else {
    pad.mapping[0] = 0;
  }

  pad.vibration_actuator.type = GamepadHapticActuatorType::kDualRumble;
  pad.vibration_actuator.not_null = has_ff_capability;

  pad.connected = true;
}

void GamepadPlatformDataFetcherLinux::RefreshEvdevDevice(
    udev_device* dev,
    const UdevGamepad& pad_info) {
  const int ev_index = pad_info.index;
  int js_match = JsIndexFromEvIndex(ev_index);
  if (pad_info.parent_syspath.empty()) {
    if (js_match >= 0)
      joydev_to_evdev_[js_match] = -1;
    ev_devices_.erase(ev_index);
    return;
  }

  EvDeviceInfo& ev_info = GetOrCreateEvDeviceInfo(ev_index);
  if (ev_info.parent_syspath != pad_info.parent_syspath)
    js_match = -1;

  if (js_match < 0) {
    for (size_t i = 0; i < Gamepads::kItemsLengthCap; ++i) {
      const auto& js_info = js_devices_[i];
      if (js_info && js_info->parent_syspath == pad_info.parent_syspath) {
        js_match = i;
        break;
      }
    }
  }

  CloseFileDescriptorIfValid(ev_info.fd);
  int device_fd = open(pad_info.path.c_str(), O_RDWR | O_NONBLOCK);
  if (device_fd < 0) {
    if (js_match >= 0)
      joydev_to_evdev_[js_match] = -1;
    ev_devices_.erase(ev_index);
    return;
  }

  ev_info.fd = device_fd;
  ev_info.has_ff_capability = HasForceFeedbackCapability(device_fd);
  ev_info.parent_syspath = pad_info.parent_syspath;

  if (js_match >= 0)
    joydev_to_evdev_[js_match] = ev_index;
}

void GamepadPlatformDataFetcherLinux::EnumerateDevices() {
  if (!udev_->udev_handle())
    return;
  ScopedUdevEnumeratePtr enumerate(udev_enumerate_new(udev_->udev_handle()));
  if (!enumerate)
    return;
  int ret =
      udev_enumerate_add_match_subsystem(enumerate.get(), kInputSubsystem);
  if (ret != 0)
    return;
  ret = udev_enumerate_scan_devices(enumerate.get());
  if (ret != 0)
    return;

  std::fill(joydev_to_evdev_, joydev_to_evdev_ + Gamepads::kItemsLengthCap, -1);

  udev_list_entry* devices = udev_enumerate_get_list_entry(enumerate.get());
  for (udev_list_entry* dev_list_entry = devices; dev_list_entry != nullptr;
       dev_list_entry = udev_list_entry_get_next(dev_list_entry)) {
    // Get the filename of the /sys entry for the device and create a
    // udev_device object (dev) representing it
    const char* path = udev_list_entry_get_name(dev_list_entry);
    ScopedUdevDevicePtr dev(
        udev_device_new_from_syspath(udev_->udev_handle(), path));
    if (!dev)
      continue;
    RefreshDevice(dev.get());
  }
}

void GamepadPlatformDataFetcherLinux::ReadDeviceData(size_t index) {
  // Linker does not like CHECK_LT(index, Gamepads::kItemsLengthCap). =/
  if (index >= Gamepads::kItemsLengthCap) {
    CHECK(false);
    return;
  }
  DCHECK(js_devices_[index]);

  PadState* state = GetPadState(index);
  if (!state)
    return;

  int fd = js_devices_[index]->fd;
  DCHECK_GE(fd, 0);

  Gamepad& pad = state->data;

  js_event event;
  while (HANDLE_EINTR(read(fd, &event, sizeof(struct js_event))) > 0) {
    size_t item = event.number;
    if (event.type & JS_EVENT_AXIS) {
      if (item >= Gamepad::kAxesLengthCap)
        continue;

      pad.axes[item] = event.value / kMaxLinuxAxisValue;

      if (item >= pad.axes_length)
        pad.axes_length = item + 1;
    } else if (event.type & JS_EVENT_BUTTON) {
      if (item >= Gamepad::kButtonsLengthCap)
        continue;

      pad.buttons[item].pressed = event.value;
      pad.buttons[item].value = event.value ? 1.0 : 0.0;

      if (item >= pad.buttons_length)
        pad.buttons_length = item + 1;
    }
    pad.timestamp = event.time;
  }
}

void GamepadPlatformDataFetcherLinux::PlayEffect(
    int pad_id,
    mojom::GamepadHapticEffectType type,
    mojom::GamepadEffectParametersPtr params,
    mojom::GamepadHapticsManager::PlayVibrationEffectOnceCallback callback) {
  if (pad_id < 0 || pad_id >= static_cast<int>(Gamepads::kItemsLengthCap)) {
    std::move(callback).Run(
        mojom::GamepadHapticsResult::GamepadHapticsResultError);
    return;
  }

  if (type !=
      mojom::GamepadHapticEffectType::GamepadHapticEffectTypeDualRumble) {
    // Only dual-rumble effects are supported.
    std::move(callback).Run(
        mojom::GamepadHapticsResult::GamepadHapticsResultNotSupported);
    return;
  }

  EvDeviceInfo* ev_info = EvDeviceInfoFromJsIndex(pad_id);
  if (!ev_info || ev_info->fd < 0 || !ev_info->has_ff_capability) {
    // No evdev device found for this gamepad, or the evdev device has no force
    // feedback support.
    std::move(callback).Run(
        mojom::GamepadHapticsResult::GamepadHapticsResultNotSupported);
    return;
  }

  sequence_ids_[pad_id]++;

  if (pending_callbacks_[pad_id]) {
    std::move(pending_callbacks_[pad_id])
        .Run(mojom::GamepadHapticsResult::GamepadHapticsResultPreempted);
  }
  pending_callbacks_[pad_id] = std::move(callback);

  PlayDualRumbleEffect(*ev_info, pad_id, params->duration, params->start_delay,
                       params->strong_magnitude, params->weak_magnitude);
}

void GamepadPlatformDataFetcherLinux::ResetVibration(
    int pad_id,
    mojom::GamepadHapticsManager::ResetVibrationActuatorCallback callback) {
  if (pad_id < 0 || pad_id >= static_cast<int>(Gamepads::kItemsLengthCap)) {
    std::move(callback).Run(
        mojom::GamepadHapticsResult::GamepadHapticsResultError);
    return;
  }

  sequence_ids_[pad_id]++;

  if (pending_callbacks_[pad_id]) {
    EvDeviceInfo* ev_info = EvDeviceInfoFromJsIndex(pad_id);
    if (ev_info && ev_info->fd >= 0 && ev_info->effect_id != kInvalidEffectId)
      StartOrStopEffect(ev_info->fd, ev_info->effect_id, false);
    std::move(pending_callbacks_[pad_id])
        .Run(mojom::GamepadHapticsResult::GamepadHapticsResultPreempted);
  }

  std::move(callback).Run(
      mojom::GamepadHapticsResult::GamepadHapticsResultComplete);
}

void GamepadPlatformDataFetcherLinux::PlayDualRumbleEffect(
    EvDeviceInfo& ev_info,
    int pad_id,
    double duration,
    double start_delay,
    double strong_magnitude,
    double weak_magnitude) {
  uint16_t duration_millis = static_cast<uint16_t>(duration);
  uint16_t start_delay_millis = static_cast<uint16_t>(start_delay);
  uint16_t strong_magnitude_scaled =
      static_cast<uint16_t>(strong_magnitude * kRumbleMagnitudeMax);
  uint16_t weak_magnitude_scaled =
      static_cast<uint16_t>(weak_magnitude * kRumbleMagnitudeMax);

  // Upload the effect and get the new effect ID. If we already created an
  // effect on this device, reuse its ID.
  ev_info.effect_id = StoreRumbleEffect(
      ev_info.fd, ev_info.effect_id, duration_millis, start_delay_millis,
      strong_magnitude_scaled, weak_magnitude_scaled);
  if (ev_info.effect_id == kInvalidEffectId) {
    std::move(pending_callbacks_[pad_id])
        .Run(mojom::GamepadHapticsResult::GamepadHapticsResultError);
    return;
  }

  // Play the effect.
  if (!StartOrStopEffect(ev_info.fd, ev_info.effect_id, true)) {
    std::move(pending_callbacks_[pad_id])
        .Run(mojom::GamepadHapticsResult::GamepadHapticsResultError);
    return;
  }

  base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(
      FROM_HERE,
      base::BindOnce(&GamepadPlatformDataFetcherLinux::FinishEffect,
                     base::Unretained(this), sequence_ids_[pad_id], pad_id),
      base::TimeDelta::FromMillisecondsD(duration));
}

void GamepadPlatformDataFetcherLinux::FinishEffect(int sequence_id,
                                                   int pad_id) {
  if (sequence_id == sequence_ids_[pad_id]) {
    std::move(pending_callbacks_[pad_id])
        .Run(mojom::GamepadHapticsResult::GamepadHapticsResultComplete);
  }
}

JsDeviceInfo& GamepadPlatformDataFetcherLinux::GetOrCreateJsDeviceInfo(
    int js_index) {
  auto& js_info = js_devices_[js_index];
  if (!js_info) {
    js_info = base::MakeUnique<JsDeviceInfo>();
    js_info->fd = -1;
  }
  return *js_info.get();
}

EvDeviceInfo& GamepadPlatformDataFetcherLinux::GetOrCreateEvDeviceInfo(
    int ev_index) {
  auto find_it = ev_devices_.find(ev_index);
  if (find_it != ev_devices_.end())
    return *find_it->second.get();

  auto ev_info = base::MakeUnique<EvDeviceInfo>();
  ev_info->fd = -1;
  ev_info->effect_id = kInvalidEffectId;
  ev_info->has_ff_capability = false;
  auto emplace_result = ev_devices_.emplace(ev_index, std::move(ev_info));
  return *emplace_result.first->second.get();
}

int GamepadPlatformDataFetcherLinux::JsIndexFromEvIndex(int ev_index) {
  auto* find_it = std::find(
      joydev_to_evdev_, joydev_to_evdev_ + Gamepads::kItemsLengthCap, ev_index);
  if (find_it >= joydev_to_evdev_ + Gamepads::kItemsLengthCap)
    return -1;
  return std::distance(joydev_to_evdev_, find_it);
}

EvDeviceInfo* GamepadPlatformDataFetcherLinux::EvDeviceInfoFromJsIndex(
    int js_index) {
  if (js_index < 0 || js_index >= (int)Gamepads::kItemsLengthCap)
    return nullptr;

  int ev_index = joydev_to_evdev_[js_index];
  if (ev_index < 0)
    return nullptr;

  auto find_it = ev_devices_.find(ev_index);
  if (find_it == ev_devices_.end())
    return nullptr;

  return find_it->second.get();
}

}  // namespace device
