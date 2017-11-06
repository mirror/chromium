// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_GAMEPAD_GAMEPAD_PLATFORM_DATA_FETCHER_MAC_H_
#define DEVICE_GAMEPAD_GAMEPAD_PLATFORM_DATA_FETCHER_MAC_H_

#include <CoreFoundation/CoreFoundation.h>
#include <ForceFeedback/ForceFeedback.h>
#include <IOKit/hid/IOHIDManager.h>
#include <stddef.h>

#include <memory>

#include "base/compiler_specific.h"
#include "base/mac/scoped_cftyperef.h"
#include "base/macros.h"
#include "base/sequenced_task_runner.h"
#include "build/build_config.h"
#include "device/gamepad/gamepad_data_fetcher.h"
#include "device/gamepad/public/cpp/gamepad.h"

#if defined(__OBJC__)
@class NSArray;
#else
class NSArray;
#endif

namespace device {

class GamepadPlatformDataFetcherMac : public GamepadDataFetcher {
 public:
  typedef GamepadDataFetcherFactoryImpl<GamepadPlatformDataFetcherMac,
                                        GAMEPAD_SOURCE_MAC_HID>
      Factory;

  GamepadPlatformDataFetcherMac();
  ~GamepadPlatformDataFetcherMac() override;

  GamepadSource source() override;

  void GetGamepadData(bool devices_changed_hint) override;
  void PauseHint(bool paused) override;
  void PlayEffect(
      int source_id,
      mojom::GamepadHapticEffectType,
      mojom::GamepadEffectParametersPtr,
      mojom::GamepadHapticsManager::PlayVibrationEffectOnceCallback) override;
  void ResetVibration(
      int source_id,
      mojom::GamepadHapticsManager::ResetVibrationActuatorCallback) override;

 private:
  bool enabled_;
  bool paused_;
  base::ScopedCFTypeRef<IOHIDManagerRef> hid_manager_ref_;

  static GamepadPlatformDataFetcherMac* InstanceFromContext(void* context);
  static void DeviceAddCallback(void* context,
                                IOReturn result,
                                void* sender,
                                IOHIDDeviceRef ref);
  static void DeviceRemoveCallback(void* context,
                                   IOReturn result,
                                   void* sender,
                                   IOHIDDeviceRef ref);
  static void ValueChangedCallback(void* context,
                                   IOReturn result,
                                   void* sender,
                                   IOHIDValueRef ref);
  static void DoRunCallback(
      mojom::GamepadHapticsManager::PlayVibrationEffectOnceCallback callback,
      mojom::GamepadHapticsResult result);

  void OnAddedToProvider() override;

  size_t GetEmptySlot();
  size_t GetSlotForDevice(IOHIDDeviceRef device);
  size_t SlotForLocation(int location_id);

  void DeviceAdd(IOHIDDeviceRef device);
  bool CheckCollection(IOHIDElementRef element);
  bool AddButtonsAndAxes(NSArray* elements, PadState* state, size_t slot);
  void DeviceRemove(IOHIDDeviceRef device);
  void ValueChanged(IOHIDValueRef value);

  void RegisterForNotifications();
  void UnregisterFromNotifications();

  void PlayDualRumbleEffect(int sequence_id,
                            size_t slot,
                            double duration,
                            double start_delay,
                            double strong_magnitude,
                            double weak_magnitude);
  void FinishEffect(int sequence_id, size_t slot);

  void RunCallbackOnMojoThread(size_t slot, mojom::GamepadHapticsResult result);

  // Side-band data that's not passed to the consumer.
  struct AssociatedData {
    int location_id;
    IOHIDDeviceRef device_ref;
    IOHIDElementRef button_elements[Gamepad::kButtonsLengthCap];
    IOHIDElementRef axis_elements[Gamepad::kAxesLengthCap];
    CFIndex axis_minimums[Gamepad::kAxesLengthCap];
    CFIndex axis_maximums[Gamepad::kAxesLengthCap];
    CFIndex axis_report_sizes[Gamepad::kAxesLengthCap];

    // Force feedback data
    FFDeviceObjectReference ff_device_ref;
    FFEffectObjectReference ff_effect_ref;
    FFEFFECT ff_effect;
    FFCUSTOMFORCE ff_custom_force;
    LONG force_data[2];
    DWORD axes_data[2];
    LONG direction_data[2];
    int sequence_id;
  };
  AssociatedData associated_[Gamepads::kItemsLengthCap];
  scoped_refptr<base::SequencedTaskRunner>
      callback_runners_[Gamepads::kItemsLengthCap];
  mojom::GamepadHapticsManager::PlayVibrationEffectOnceCallback
      playing_effect_callbacks_[Gamepads::kItemsLengthCap];

  DISALLOW_COPY_AND_ASSIGN(GamepadPlatformDataFetcherMac);
};

}  // namespace device

#endif  // DEVICE_GAMEPAD_GAMEPAD_PLATFORM_DATA_FETCHER_MAC_H_
