// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_EVENTS_DEVICES_DEVICE_DATA_MANAGER_H_
#define UI_EVENTS_DEVICES_DEVICE_DATA_MANAGER_H_

#include <stdint.h>

#include <array>
#include <memory>
#include <vector>

#include "base/macros.h"
#include "base/observer_list.h"
#include "ui/events/devices/device_hotplug_event_observer.h"
#include "ui/events/devices/events_devices_export.h"
#include "ui/events/devices/input_device_manager.h"
#include "ui/events/devices/touch_device_transform.h"
#include "ui/events/devices/touchscreen_device.h"

namespace ui {

namespace test {
class DeviceDataManagerTestAPI;
}  // namespace test

class InputDeviceEventObserver;

// Keeps track of device mappings and event transformations.
class EVENTS_DEVICES_EXPORT DeviceDataManager
    : public InputDeviceManager,
      public DeviceHotplugEventObserver {
 public:
  static const int kMaxDeviceNum = 128;
  ~DeviceDataManager() override;

  static void CreateInstance();
  static void DeleteInstance();
  static DeviceDataManager* GetInstance();
  static bool HasInstance();

  // Configures the touch devices. |transforms| contains the transform for each
  // device and display pair.
  void ConfigureTouchDevices(
      const std::vector<ui::TouchDeviceTransform>& transforms);

  // This will temporarily associate the touch device having |touch_device_id|
  // and the display having id as |display_under_calibration_| until the next
  // call to |ConfigureTouchDevices()|.
  void AssociateTouchDeviceForCalibration(int touch_device_id);

  // Initializes the parameters for touch device association. Initializes
  // |transform_for_touch_device_under_calibration_| that will be associated w
  // ith the touch device being used during touch calibration.
  void PrepareForTouchDeviceAssociation(
      const ui::TouchDeviceTransform& transform,
      int64_t internal_display_id);

  // Resets params associated with Touch association.
  void ResetTouchCalibrationParams();

  bool require_touch_association() const { return require_touch_association_; }

  void ApplyTouchTransformer(int touch_device_id, float* x, float* y);

  // Gets the display that touches from |touch_device_id| should be sent to.
  int64_t GetTargetDisplayForTouchDevice(int touch_device_id) const;

  void ApplyTouchRadiusScale(int touch_device_id, double* radius);

  void SetTouchscreensEnabled(bool enabled);

  // InputDeviceManager:
  const std::vector<TouchscreenDevice>& GetTouchscreenDevices() const override;
  const std::vector<InputDevice>& GetKeyboardDevices() const override;
  const std::vector<InputDevice>& GetMouseDevices() const override;
  const std::vector<InputDevice>& GetTouchpadDevices() const override;
  bool AreDeviceListsComplete() const override;
  bool AreTouchscreensEnabled() const override;
  bool AreTouchscreenTargetDisplaysValid() const override;

  void AddObserver(InputDeviceEventObserver* observer) override;
  void RemoveObserver(InputDeviceEventObserver* observer) override;

 protected:
  DeviceDataManager();

  static void set_instance(DeviceDataManager* instance);

  // DeviceHotplugEventObserver:
  void OnTouchscreenDevicesUpdated(
      const std::vector<TouchscreenDevice>& devices) override;
  void OnKeyboardDevicesUpdated(
      const std::vector<InputDevice>& devices) override;
  void OnMouseDevicesUpdated(
      const std::vector<InputDevice>& devices) override;
  void OnTouchpadDevicesUpdated(
      const std::vector<InputDevice>& devices) override;
  void OnDeviceListsComplete() override;
  void OnStylusStateChanged(StylusState state) override;

 private:
  friend class test::DeviceDataManagerTestAPI;

  void ClearTouchDeviceAssociations();
  void UpdateTouchInfoFromTransform(
      const ui::TouchDeviceTransform& touch_device_transform);
  bool IsTouchDeviceIdValid(int touch_device_id) const;

  void NotifyObserversTouchscreenDeviceConfigurationChanged();
  void NotifyObserversKeyboardDeviceConfigurationChanged();
  void NotifyObserversMouseDeviceConfigurationChanged();
  void NotifyObserversTouchpadDeviceConfigurationChanged();
  void NotifyObserversDeviceListsComplete();
  void NotifyObserversStylusStateChanged(StylusState stylus_state);

  static DeviceDataManager* instance_;

  std::vector<TouchscreenDevice> touchscreen_devices_;
  std::vector<InputDevice> keyboard_devices_;
  std::vector<InputDevice> mouse_devices_;
  std::vector<InputDevice> touchpad_devices_;
  bool device_lists_complete_ = false;

  base::ObserverList<InputDeviceEventObserver> observers_;

  bool touch_screens_enabled_ = true;

  // Set to true when ConfigureTouchDevices() is called.
  bool are_touchscreen_target_displays_valid_ = false;

  // Contains touchscreen device info for each device mapped by device ID. Will
  // have default values if the device with corresponding ID isn't a touchscreen
  // or doesn't exist.
  std::array<TouchDeviceTransform, kMaxDeviceNum> touch_map_;

  // The id of the display that is undergoing touch calibration. This helps
  // associate touch devices that are not associated with any displays.
  // This will have a valid display id when the display that is under going
  // touch calibration does not have a touch device associated with it. This
  // is only valid if |require_touch_association_| is true.
  // The |device_id| field stores the internal touch device's id.
  ui::TouchDeviceTransform transform_for_touch_device_under_calibration_;

  // Is true when the display that is undergoing touch calibration does not have
  // a touch device associated with it.
  bool require_touch_association_ = false;

  DISALLOW_COPY_AND_ASSIGN(DeviceDataManager);
};

}  // namespace ui

#endif  // UI_EVENTS_DEVICES_DEVICE_DATA_MANAGER_H_
