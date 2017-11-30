// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_GAMEPAD_GAMEPAD_PLATFORM_DATA_FETCHER_LINUX_H_
#define DEVICE_GAMEPAD_GAMEPAD_PLATFORM_DATA_FETCHER_LINUX_H_

#include <stddef.h>

#include <memory>
#include <string>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "device/gamepad/gamepad_data_fetcher.h"
#include "device/gamepad/public/cpp/gamepads.h"

extern "C" {
struct udev_device;
}

namespace device {
class UdevLinux;
}

namespace device {

struct JsDeviceInfo {
  int fd;
  int vendor_id;
  int product_id;
  std::string parent_syspath;
};

struct EvDeviceInfo {
  int fd;
  int effect_id;
  bool has_ff_capability;
  std::string parent_syspath;
};

struct HidrawDeviceInfo {
  int fd;
  std::string syspath;
};

enum class UdevGamepadType {
  JOYDEV,
  EVDEV,
  HIDRAW,
};

struct UdevGamepad {
  UdevGamepad();
  ~UdevGamepad();

  UdevGamepadType type;
  int index;
  std::string path;
  std::string syspath;
  std::string parent_syspath;
};

class DEVICE_GAMEPAD_EXPORT GamepadPlatformDataFetcherLinux
    : public GamepadDataFetcher {
 public:
  using Factory = GamepadDataFetcherFactoryImpl<GamepadPlatformDataFetcherLinux,
                                                GAMEPAD_SOURCE_LINUX_UDEV>;

  GamepadPlatformDataFetcherLinux();
  ~GamepadPlatformDataFetcherLinux() override;

  GamepadSource source() override;

  // GamepadDataFetcher implementation.
  void GetGamepadData(bool devices_changed_hint) override;

  void PlayEffect(
      int pad_index,
      mojom::GamepadHapticEffectType,
      mojom::GamepadEffectParametersPtr,
      mojom::GamepadHapticsManager::PlayVibrationEffectOnceCallback) override;

  void ResetVibration(
      int pad_index,
      mojom::GamepadHapticsManager::ResetVibrationActuatorCallback) override;

 private:
  void OnAddedToProvider() override;

  void PlayEffectDualshock4(
      int pad_index,
      mojom::GamepadHapticEffectType,
      mojom::GamepadEffectParametersPtr,
      mojom::GamepadHapticsManager::PlayVibrationEffectOnceCallback);
  void ResetVibrationDualshock4(
      int pad_index,
      mojom::GamepadHapticsManager::ResetVibrationActuatorCallback);

  void RefreshDevice(udev_device* dev);
  void RefreshJoydevDevice(udev_device* dev, const UdevGamepad& pad_info);
  void RefreshEvdevDevice(udev_device* dev, const UdevGamepad& pad_info);
  void RefreshHidrawDevice(udev_device* dev, const UdevGamepad& pad_info);
  void EnumerateDevices();
  void EnumerateInputDevices();
  void EnumerateHidrawDevices();
  void ReadDeviceData(size_t index);
  void PlayDualRumbleEffect(EvDeviceInfo& ev_info,
                            int pad_id,
                            double duration,
                            double start_delay,
                            double strong_magnitude,
                            double weak_magnitude);
  void FinishEffect(int sequence_id, int pad_id);
  void PlayDualRumbleEffectDualshock4(HidrawDeviceInfo& hidraw_info,
                                      int pad_id,
                                      double duration,
                                      double start_delay,
                                      double strong_magnitude,
                                      double weak_magnitude);
  void StartDualshock4Vibration(int sequence_id,
                                int pad_id,
                                double duration,
                                double strong_magnitude,
                                double weak_magnitude);
  void StopDualshock4Vibration(int sequence_id, int pad_id);
  void SetDualshock4Vibration(int pad_id,
                              double strong_magnitude,
                              double weak_magnitude);
  JsDeviceInfo& GetOrCreateJsDeviceInfo(int js_index);
  EvDeviceInfo& GetOrCreateEvDeviceInfo(int ev_index);
  HidrawDeviceInfo& GetOrCreateHidrawDeviceInfo(int hidraw_index);
  int JsIndexFromEvIndex(int ev_index);
  int JsIndexFromHidrawIndex(int hidraw_index);
  EvDeviceInfo* EvDeviceInfoFromJsIndex(int js_index);
  HidrawDeviceInfo* HidrawDeviceInfoFromJsIndex(int js_index);

  // The evdev device indices for each connected joydev device, or -1 if there
  // is no associated evdev device.
  int joydev_to_evdev_[Gamepads::kItemsLengthCap];

  // The hidraw device indices for each connected joydev device, or -1 if there
  // is no associated hidraw device.
  int joydev_to_hidraw_[Gamepads::kItemsLengthCap];

  // Sequence IDs are stored for each gamepad to allow previous effect sequences
  // to be preempted by new sequences.
  int sequence_ids_[Gamepads::kItemsLengthCap];

  // Completion callbacks for playing effects. The completion callback is called
  // when an effect finishes playback or is preempted by another sequence.
  mojom::GamepadHapticsManager::PlayVibrationEffectOnceCallback
      pending_callbacks_[Gamepads::kItemsLengthCap];

  // Device info for /dev/input/js* (joydev) devices. nullptr when not in use.
  std::unique_ptr<JsDeviceInfo> js_devices_[Gamepads::kItemsLengthCap];

  // Device info for /dev/input/event* (evdev) devices, keyed by device index.
  std::unordered_map<int, std::unique_ptr<EvDeviceInfo>> ev_devices_;

  // Device info for /dev/hidraw* (hidraw) devices, keyed by device index.
  std::unordered_map<int, std::unique_ptr<HidrawDeviceInfo>> hidraw_devices_;

  std::unique_ptr<device::UdevLinux> udev_;

  DISALLOW_COPY_AND_ASSIGN(GamepadPlatformDataFetcherLinux);
};

}  // namespace device

#endif  // DEVICE_GAMEPAD_GAMEPAD_PLATFORM_DATA_FETCHER_LINUX_H_
