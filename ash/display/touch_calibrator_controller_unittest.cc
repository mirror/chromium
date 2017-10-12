// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/display/touch_calibrator_controller.h"

#include <vector>

#include "ash/display/touch_calibrator_view.h"
#include "ash/shell.h"
#include "ash/test/ash_test_base.h"
#include "base/stl_util.h"
#include "ui/display/display.h"
#include "ui/display/manager/display_manager.h"
#include "ui/display/test/display_manager_test_api.h"
#include "ui/events/base_event_utils.h"
#include "ui/events/devices/device_data_manager.h"
#include "ui/events/event_handler.h"
#include "ui/events/test/device_data_manager_test_api.h"
#include "ui/events/test/event_generator.h"
#include "ui/events/test/events_test_utils.h"

using namespace display;

namespace ash {
namespace {

ui::TouchscreenDevice GetInternalTouchDevice(int touch_device_id) {
  return ui::TouchscreenDevice(
      touch_device_id, ui::InputDeviceType::INPUT_DEVICE_INTERNAL,
      std::string("test touch device"), gfx::Size(1000, 1000), 1);
}

ui::TouchscreenDevice GetExternalTouchDevice(int touch_device_id) {
  return ui::TouchscreenDevice(
      touch_device_id, ui::InputDeviceType::INPUT_DEVICE_EXTERNAL,
      std::string("test touch device"), gfx::Size(1000, 1000), 1);
}

}  // namespace

class TouchCalibratorControllerTest : public AshTestBase {
 public:
  TouchCalibratorControllerTest() {}

  const Display& InitDisplays() {
    // Initialize 2 displays each with resolution 500x500.
    UpdateDisplay("500x500,500x500");
    // Assuming index 0 points to the native display, we will calibrate the
    // touch display at index 1.
    const int kTargetDisplayIndex = 1;
    DisplayIdList display_id_list =
        display_manager()->GetCurrentDisplayIdList();
    int64_t target_display_id = display_id_list[kTargetDisplayIndex];
    const Display& touch_display =
        display_manager()->GetDisplayForId(target_display_id);
    return touch_display;
  }

  void StartCalibrationChecks(TouchCalibratorController* ctrl,
                              const Display& target_display) {
    EXPECT_FALSE(ctrl->IsCalibrating());
    EXPECT_FALSE(!!ctrl->touch_calibrator_views_.size());

    TouchCalibratorController::TouchCalibrationCallback empty_callback;

    ctrl->StartCalibration(target_display, false /* is_custom_calibration */,
                           std::move(empty_callback));
    EXPECT_TRUE(ctrl->IsCalibrating());
    EXPECT_EQ(ctrl->state_,
              TouchCalibratorController::CalibrationState::kNativeCalibration);

    // There should be a touch calibrator view associated with each of the
    // active displays.
    EXPECT_EQ(ctrl->touch_calibrator_views_.size(),
              display_manager()->GetCurrentDisplayIdList().size());

    TouchCalibratorView* target_calibrator_view =
        ctrl->touch_calibrator_views_[target_display.id()].get();

    // End the background fade in animation.
    target_calibrator_view->SkipCurrentAnimation();

    // TouchCalibratorView on the display being calibrated should be at the
    // state where the first display point is visible.
    EXPECT_EQ(target_calibrator_view->state(),
              TouchCalibratorView::DISPLAY_POINT_1);
  }

  void ResetTimestampThreshold(TouchCalibratorController* ctrl) {
    base::Time current = base::Time::Now();
    ctrl->last_touch_timestamp_ =
        current - (TouchCalibratorController::kTouchIntervalThreshold);
  }

  // Generates a touch press and release event in the |display| with source
  // device id as |touch_device_id|.
  void GenerateTouchEvent(const Display& display,
                          int touch_device_id,
                          const gfx::Point& location = gfx::Point(20, 20)) {
    gfx::Vector2d offset(display_manager()
                             ->GetDisplayInfo(display.id())
                             .bounds_in_native()
                             .OffsetFromOrigin());
    ui::test::EventGenerator& eg = GetEventGenerator();
    ui::TouchEvent press_touch_event(
        ui::ET_TOUCH_PRESSED, location + offset, ui::EventTimeForNow(),
        ui::PointerDetails(ui::EventPointerType::POINTER_TYPE_TOUCH, 12, 1.0f,
                           1.0f, 0.0f),
        0, 0.0f);
    ui::TouchEvent release_touch_event(
        ui::ET_TOUCH_RELEASED, location + offset, ui::EventTimeForNow(),
        ui::PointerDetails(ui::EventPointerType::POINTER_TYPE_TOUCH, 12, 1.0f,
                           1.0f, 0.0f),
        0, 0.0f);

    press_touch_event.set_source_device_id(touch_device_id);
    release_touch_event.set_source_device_id(touch_device_id);

    eg.Dispatch(&press_touch_event);
    eg.Dispatch(&release_touch_event);
  }

  ui::TouchscreenDevice InitTouchDevice(
      int64_t display_id,
      const ui::TouchscreenDevice& touchdevice) {
    ui::DeviceDataManager::CreateInstance();

    ui::test::DeviceDataManagerTestAPI devices_test_api;
    devices_test_api.SetTouchscreenDevices({touchdevice});

    std::vector<ui::TouchDeviceTransform> transforms;
    ui::TouchDeviceTransform touch_device_transform;
    touch_device_transform.display_id = display_id;
    touch_device_transform.device_id = touchdevice.id;
    transforms.push_back(touch_device_transform);

    ui::DeviceDataManager::GetInstance()->ConfigureTouchDevices(transforms);
    return touchdevice;
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(TouchCalibratorControllerTest);
};

TEST_F(TouchCalibratorControllerTest, StartCalibration) {
  const Display& touch_display = InitDisplays();
  TouchCalibratorController touch_calibrator_controller;
  StartCalibrationChecks(&touch_calibrator_controller, touch_display);

  ui::EventTargetTestApi test_api(Shell::Get());
  const ui::EventHandlerList& handlers = test_api.pre_target_handlers();
  EXPECT_TRUE(base::ContainsValue(handlers, &touch_calibrator_controller));
}

TEST_F(TouchCalibratorControllerTest, KeyEventIntercept) {
  const Display& touch_display = InitDisplays();
  TouchCalibratorController touch_calibrator_controller;
  StartCalibrationChecks(&touch_calibrator_controller, touch_display);

  ui::test::EventGenerator& eg = GetEventGenerator();
  EXPECT_TRUE(touch_calibrator_controller.IsCalibrating());
  eg.PressKey(ui::VKEY_ESCAPE, ui::EF_NONE);
  EXPECT_FALSE(touch_calibrator_controller.IsCalibrating());
}

TEST_F(TouchCalibratorControllerTest, TouchThreshold) {
  const Display& touch_display = InitDisplays();
  TouchCalibratorController touch_calibrator_controller;
  StartCalibrationChecks(&touch_calibrator_controller, touch_display);

  // Initialize touch device so that |event_transnformer_| can be computed.
  ui::TouchscreenDevice touchdevice =
      InitTouchDevice(touch_display.id(), GetExternalTouchDevice(12));

  ui::test::EventGenerator& eg = GetEventGenerator();

  base::Time current_timestamp = base::Time::Now();
  touch_calibrator_controller.last_touch_timestamp_ = current_timestamp;

  eg.set_current_location(gfx::Point(0, 0));
  eg.PressTouch();
  eg.ReleaseTouch();

  EXPECT_EQ(touch_calibrator_controller.last_touch_timestamp_,
            current_timestamp);

  ResetTimestampThreshold(&touch_calibrator_controller);

  eg.PressTouch();
  eg.ReleaseTouch();

  EXPECT_LT(current_timestamp,
            touch_calibrator_controller.last_touch_timestamp_);
}

TEST_F(TouchCalibratorControllerTest, TouchDeviceIdIsSet) {
  const Display& touch_display = InitDisplays();

  TouchCalibratorController touch_calibrator_controller;
  StartCalibrationChecks(&touch_calibrator_controller, touch_display);

  ui::TouchscreenDevice touchdevice =
      InitTouchDevice(touch_display.id(), GetExternalTouchDevice(12));

  ResetTimestampThreshold(&touch_calibrator_controller);
  EXPECT_EQ(touch_calibrator_controller.touch_device_id_,
            ui::InputDevice::kInvalidId);
  GenerateTouchEvent(touch_display, touchdevice.id);
  EXPECT_EQ(touch_calibrator_controller.touch_device_id_, touchdevice.id);
}

TEST_F(TouchCalibratorControllerTest, CustomCalibration) {
  const Display& touch_display = InitDisplays();

  TouchCalibratorController touch_calibrator_controller;
  EXPECT_FALSE(touch_calibrator_controller.IsCalibrating());
  EXPECT_FALSE(!!touch_calibrator_controller.touch_calibrator_views_.size());

  touch_calibrator_controller.StartCalibration(
      touch_display, true /* is_custom_calibbration */,
      TouchCalibratorController::TouchCalibrationCallback());

  ui::TouchscreenDevice touchdevice =
      InitTouchDevice(touch_display.id(), GetExternalTouchDevice(12));

  EXPECT_TRUE(touch_calibrator_controller.IsCalibrating());
  EXPECT_EQ(touch_calibrator_controller.state_,
            TouchCalibratorController::CalibrationState::kCustomCalibration);

  // Native touch calibration UX should not initialize during custom calibration
  EXPECT_EQ(touch_calibrator_controller.touch_calibrator_views_.size(), 0UL);
  EXPECT_EQ(touch_calibrator_controller.touch_device_id_,
            ui::InputDevice::kInvalidId);

  ResetTimestampThreshold(&touch_calibrator_controller);

  GenerateTouchEvent(touch_display, touchdevice.id);
  EXPECT_EQ(touch_calibrator_controller.touch_device_id_, touchdevice.id);

  display::TouchCalibrationData::CalibrationPointPairQuad points = {
      {std::make_pair(gfx::Point(10, 10), gfx::Point(11, 12)),
       std::make_pair(gfx::Point(190, 10), gfx::Point(195, 8)),
       std::make_pair(gfx::Point(10, 90), gfx::Point(12, 94)),
       std::make_pair(gfx::Point(190, 90), gfx::Point(189, 88))}};
  gfx::Size size(200, 100);
  display::TouchCalibrationData calibration_data(points, size);

  touch_calibrator_controller.CompleteCalibration(points, size);

  const display::ManagedDisplayInfo& info =
      display_manager()->GetDisplayInfo(touch_display.id());

  uint32_t touch_device_identifier =
      display::TouchCalibrationData::GenerateTouchDeviceIdentifier(touchdevice);
  EXPECT_TRUE(info.HasTouchCalibrationData(touch_device_identifier));
  EXPECT_EQ(calibration_data,
            info.GetTouchCalibrationData(touch_device_identifier));
}

TEST_F(TouchCalibratorControllerTest, CustomCalibrationInvalidTouchId) {
  const Display& touch_display = InitDisplays();

  TouchCalibratorController touch_calibrator_controller;
  EXPECT_FALSE(touch_calibrator_controller.IsCalibrating());
  EXPECT_FALSE(!!touch_calibrator_controller.touch_calibrator_views_.size());

  touch_calibrator_controller.StartCalibration(
      touch_display, true /* is_custom_calibbration */,
      TouchCalibratorController::TouchCalibrationCallback());

  EXPECT_TRUE(touch_calibrator_controller.IsCalibrating());
  EXPECT_EQ(touch_calibrator_controller.state_,
            TouchCalibratorController::CalibrationState::kCustomCalibration);

  // Native touch calibration UX should not initialize during custom calibration
  EXPECT_EQ(touch_calibrator_controller.touch_calibrator_views_.size(), 0UL);
  EXPECT_EQ(touch_calibrator_controller.touch_device_id_,
            ui::InputDevice::kInvalidId);

  ResetTimestampThreshold(&touch_calibrator_controller);

  display::TouchCalibrationData::CalibrationPointPairQuad points = {
      {std::make_pair(gfx::Point(10, 10), gfx::Point(11, 12)),
       std::make_pair(gfx::Point(190, 10), gfx::Point(195, 8)),
       std::make_pair(gfx::Point(10, 90), gfx::Point(12, 94)),
       std::make_pair(gfx::Point(190, 90), gfx::Point(189, 88))}};
  gfx::Size size(200, 100);
  display::TouchCalibrationData calibration_data(points, size);

  touch_calibrator_controller.CompleteCalibration(points, size);

  const display::ManagedDisplayInfo& info =
      display_manager()->GetDisplayInfo(touch_display.id());

  uint32_t random_touch_device_identifier = 123456;
  EXPECT_TRUE(info.HasTouchCalibrationData(random_touch_device_identifier));
  EXPECT_EQ(calibration_data,
            info.GetTouchCalibrationData(random_touch_device_identifier));
}

TEST_F(TouchCalibratorControllerTest, IgnoreInternalTouchDevices) {
  const Display& touch_display = InitDisplays();

  // We need to initialize a touch device before starting calibration so that
  // the set |internal_touch_device_ids_| can be initialized.
  ui::TouchscreenDevice internal_touchdevice =
      InitTouchDevice(touch_display.id(), GetInternalTouchDevice(12));

  TouchCalibratorController touch_calibrator_controller;
  StartCalibrationChecks(&touch_calibrator_controller, touch_display);

  // We need to reinitialize the touch device as Starting calibration resets
  // everything.
  internal_touchdevice =
      InitTouchDevice(touch_display.id(), GetInternalTouchDevice(12));

  ResetTimestampThreshold(&touch_calibrator_controller);
  EXPECT_EQ(touch_calibrator_controller.touch_device_id_,
            ui::InputDevice::kInvalidId);
  GenerateTouchEvent(touch_display, internal_touchdevice.id);
  EXPECT_EQ(touch_calibrator_controller.touch_device_id_,
            ui::InputDevice::kInvalidId);

  ui::TouchscreenDevice external_touchdevice =
      InitTouchDevice(touch_display.id(), GetExternalTouchDevice(13));
  ResetTimestampThreshold(&touch_calibrator_controller);
  GenerateTouchEvent(touch_display, external_touchdevice.id);
  EXPECT_EQ(touch_calibrator_controller.touch_device_id_,
            external_touchdevice.id);
}

TEST_F(TouchCalibratorControllerTest, HighDPIMonitorsCalibration) {
  // Initialize 2 displays each with resolution 500x500. One of them at 2x
  // device scale factor.
  UpdateDisplay("500x500*2,500x500*1.5");

  // Index 0 points to the native internal display, we will calibrate the touch
  // display at index 1.
  const int kTargetDisplayIndex = 1;
  DisplayIdList display_id_list = display_manager()->GetCurrentDisplayIdList();

  int64_t internal_display_id = display_id_list[0];
  test::ScopedSetInternalDisplayId set_internal(display_manager(),
                                                internal_display_id);

  int64_t target_display_id = display_id_list[kTargetDisplayIndex];
  const Display& touch_display =
      display_manager()->GetDisplayForId(target_display_id);

  // We create 2 touch devices.
  const int kInternalTouchId = 10;
  const int kExternalTouchId = 11;

  ui::DeviceDataManager::CreateInstance();
  ui::TouchscreenDevice internal_touchdevice(
      kInternalTouchId, ui::InputDeviceType::INPUT_DEVICE_INTERNAL,
      std::string("internal touch device"), gfx::Size(1000, 1000), 1);
  ui::TouchscreenDevice external_touchdevice(
      kExternalTouchId, ui::InputDeviceType::INPUT_DEVICE_EXTERNAL,
      std::string("external touch device"), gfx::Size(1000, 1000), 1);

  ui::test::DeviceDataManagerTestAPI devices_test_api;
  devices_test_api.SetTouchscreenDevices(
      {internal_touchdevice, external_touchdevice});

  // Associate both touch devices to the internal display.
  std::vector<ui::TouchDeviceTransform> transforms;
  ui::TouchDeviceTransform touch_device_transform;
  touch_device_transform.display_id = internal_display_id;
  touch_device_transform.device_id = internal_touchdevice.id;
  transforms.push_back(touch_device_transform);

  touch_device_transform.device_id = external_touchdevice.id;
  transforms.push_back(touch_device_transform);

  ui::DeviceDataManager::GetInstance()->ConfigureTouchDevices(transforms);

  TouchCalibratorController touch_calibrator_controller;
  touch_calibrator_controller.StartCalibration(
      touch_display, false /* is_custom_calibbration */,
      TouchCalibratorController::TouchCalibrationCallback());

  // Skip any UI animations associated with the start of calibration.
  touch_calibrator_controller.touch_calibrator_views_[touch_display.id()]
      .get()
      ->SkipCurrentAnimation();

  // Reinitialize the transforms, as starting calibration resets them.
  ui::DeviceDataManager::GetInstance()->ConfigureTouchDevices(transforms);

  // The touch device id has not been set yet, as the first touch event has not
  // been generated.
  EXPECT_EQ(touch_calibrator_controller.touch_device_id_,
            ui::InputDevice::kInvalidId);

  ResetTimestampThreshold(&touch_calibrator_controller);

  // Generate a touch event at point (100, 100) on the external touch device.
  gfx::Point touch_point(100, 100);
  GenerateTouchEvent(touch_display, external_touchdevice.id, touch_point);
  EXPECT_EQ(touch_calibrator_controller.touch_device_id_,
            external_touchdevice.id);

  // The touch event should now be within the bounds the the target display.
  EXPECT_EQ(touch_calibrator_controller.touch_point_quad_[0].second,
            gfx::Point(101, 601));

  // The display point should have the root transform applied.
  EXPECT_EQ(touch_calibrator_controller.touch_point_quad_[0].first,
            gfx::Point(210, 210));
}

TEST_F(TouchCalibratorControllerTest, RotatedHighDPIMonitorsCalibration) {
  // Initialize 2 displays each with resolution 500x500. One of them at 2x
  // device scale factor.
  UpdateDisplay("500x500*2,500x500*1.5/r");

  // Index 0 points to the native internal display, we will calibrate the touch
  // display at index 1.
  const int kTargetDisplayIndex = 1;
  DisplayIdList display_id_list = display_manager()->GetCurrentDisplayIdList();

  int64_t internal_display_id = display_id_list[0];
  test::ScopedSetInternalDisplayId set_internal(display_manager(),
                                                internal_display_id);

  int64_t target_display_id = display_id_list[kTargetDisplayIndex];
  const Display& touch_display =
      display_manager()->GetDisplayForId(target_display_id);

  // We create 2 touch devices.
  const int kInternalTouchId = 10;
  const int kExternalTouchId = 11;

  ui::DeviceDataManager::CreateInstance();
  ui::TouchscreenDevice internal_touchdevice(
      kInternalTouchId, ui::InputDeviceType::INPUT_DEVICE_INTERNAL,
      std::string("internal touch device"), gfx::Size(1000, 1000), 1);
  ui::TouchscreenDevice external_touchdevice(
      kExternalTouchId, ui::InputDeviceType::INPUT_DEVICE_EXTERNAL,
      std::string("external touch device"), gfx::Size(1000, 1000), 1);

  ui::test::DeviceDataManagerTestAPI devices_test_api;
  devices_test_api.SetTouchscreenDevices(
      {internal_touchdevice, external_touchdevice});

  // Associate both touch devices to the internal display.
  std::vector<ui::TouchDeviceTransform> transforms;
  ui::TouchDeviceTransform touch_device_transform;
  touch_device_transform.display_id = internal_display_id;
  touch_device_transform.device_id = internal_touchdevice.id;
  transforms.push_back(touch_device_transform);

  touch_device_transform.device_id = external_touchdevice.id;
  transforms.push_back(touch_device_transform);

  ui::DeviceDataManager::GetInstance()->ConfigureTouchDevices(transforms);

  TouchCalibratorController touch_calibrator_controller;
  touch_calibrator_controller.StartCalibration(
      touch_display, false /* is_custom_calibbration */,
      TouchCalibratorController::TouchCalibrationCallback());

  // Skip any UI animations associated with the start of calibration.
  touch_calibrator_controller.touch_calibrator_views_[touch_display.id()]
      .get()
      ->SkipCurrentAnimation();

  // Reinitialize the transforms, as starting calibration resets them.
  ui::DeviceDataManager::GetInstance()->ConfigureTouchDevices(transforms);

  // The touch device id has not been set yet, as the first touch event has not
  // been generated.
  EXPECT_EQ(touch_calibrator_controller.touch_device_id_,
            ui::InputDevice::kInvalidId);

  ResetTimestampThreshold(&touch_calibrator_controller);

  // Generate a touch event at point (100, 100) on the external touch device.
  gfx::Point touch_point(100, 100);
  GenerateTouchEvent(touch_display, external_touchdevice.id, touch_point);
  EXPECT_EQ(touch_calibrator_controller.touch_device_id_,
            external_touchdevice.id);

  // The touch event should now be within the bounds the the target display.
  EXPECT_EQ(touch_calibrator_controller.touch_point_quad_[0].second,
            gfx::Point(101, 601));

  // The display point should have the root transform applied.
  EXPECT_EQ(touch_calibrator_controller.touch_point_quad_[0].first,
            gfx::Point(290, 210));
}

}  // namespace ash
