// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/display/manager/chromeos/touchscreen_util.h"

#include <memory>
#include <string>
#include <vector>

#include "base/memory/ptr_util.h"
#include "base/strings/string_number_conversions.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/display/manager/display_manager.h"
#include "ui/display/manager/managed_display_info.h"
#include "ui/display/screen_base.h"
#include "ui/display/test/display_manager_test_api.h"
#include "ui/events/devices/input_device.h"

namespace display {
namespace {

uint32_t ToIdentifier(const ui::TouchscreenDevice& device) {
  return TouchCalibrationData::GenerateTouchDeviceIdentifier(device);
}

ui::TouchscreenDevice CreateTouchscreenDevice(int id,
                                              ui::InputDeviceType type,
                                              const gfx::Size& size) {
  ui::TouchscreenDevice device(id, type, base::IntToString(id), size, 0);
  device.vendor_id = id * id;
  device.product_id = device.vendor_id * id;
  return device;
}

}  // namespace

using DisplayInfoList = std::vector<ManagedDisplayInfo>;

class TouchscreenUtilTest : public testing::Test {
 public:
  TouchscreenUtilTest() {}
  ~TouchscreenUtilTest() override {}

  DisplayManager* display_manager() { return display_manager_.get(); }

  // testing::Test:
  void SetUp() override {
    // Recreate for each test, DisplayManager has a lot of state.
    display_manager_ =
        base::MakeUnique<DisplayManager>(base::MakeUnique<ScreenBase>());

    // Internal display will always match to internal touchscreen. If internal
    // touchscreen can't be detected, it is then associated to a touch screen
    // with matching size.
    {
      ManagedDisplayInfo display(2200000000, "2200000000", false);
      scoped_refptr<ManagedDisplayMode> mode(new ManagedDisplayMode(
          gfx::Size(1920, 1080), 60.0, false /* interlaced */,
          true /* native */, 1.0 /* ui_scale */,
          1.0 /* device_scale_factor */));
      ManagedDisplayInfo::ManagedDisplayModeList modes(1, mode);
      display.SetManagedDisplayModes(modes);
      displays_.push_back(display);
    }

    {
      ManagedDisplayInfo display(2200000001, "2200000001", false);

      scoped_refptr<ManagedDisplayMode> mode(new ManagedDisplayMode(
          gfx::Size(800, 600), 60.0, false /* interlaced */, true /* native */,
          1.0 /* ui_scale */, 1.0 /* device_scale_factor */));
      ManagedDisplayInfo::ManagedDisplayModeList modes(1, mode);
      display.SetManagedDisplayModes(modes);
      displays_.push_back(display);
    }

    // Display without native mode. Must not be matched to any touch screen.
    {
      ManagedDisplayInfo display(2200000002, "2200000002", false);
      displays_.push_back(display);
    }

    {
      ManagedDisplayInfo display(2200000003, "2200000003", false);

      scoped_refptr<ManagedDisplayMode> mode(new ManagedDisplayMode(
          gfx::Size(1024, 768), 60.0, false /* interlaced */,
          /* native */ true, 1.0 /* ui_scale */,
          1.0 /* device_scale_factor */));
      ManagedDisplayInfo::ManagedDisplayModeList modes(1, mode);
      display.SetManagedDisplayModes(modes);
      displays_.push_back(display);
    }
  }

  void TearDown() override { displays_.clear(); }

 protected:
  DisplayInfoList displays_;
  std::unique_ptr<DisplayManager> display_manager_;

 private:
  DISALLOW_COPY_AND_ASSIGN(TouchscreenUtilTest);
};

TEST_F(TouchscreenUtilTest, NoTouchscreens) {
  std::vector<ui::TouchscreenDevice> devices;

  test::ScopedSetInternalDisplayId set_internal(display_manager(),
                                                displays_[0].id());
  AssociateTouchscreens(&displays_, devices, display_manager());

  for (size_t i = 0; i < displays_.size(); ++i)
    EXPECT_EQ(0u, displays_[i].TouchDevicesCount());
}

// Verify that if there are a lot of touchscreens, they will all get associated
// with a display.
TEST_F(TouchscreenUtilTest, ManyTouchscreens) {
  std::vector<ui::TouchscreenDevice> devices;
  for (int i = 0; i < 5; ++i) {
    devices.push_back(CreateTouchscreenDevice(
        i, ui::InputDeviceType::INPUT_DEVICE_EXTERNAL, gfx::Size(256, 256)));
  }

  DisplayInfoList displays;
  displays.push_back(displays_[3]);

  test::ScopedSetInternalDisplayId set_internal(display_manager(),
                                                displays_[0].id());
  AssociateTouchscreens(&displays, devices, display_manager());

  for (int i = 0; i < 5; ++i)
    EXPECT_TRUE(displays[0].HasTouchDevice(ToIdentifier(devices[i])));
}

TEST_F(TouchscreenUtilTest, OneToOneMapping) {
  std::vector<ui::TouchscreenDevice> devices;
  devices.push_back(CreateTouchscreenDevice(
      1, ui::InputDeviceType::INPUT_DEVICE_EXTERNAL, gfx::Size(800, 600)));
  devices.push_back(CreateTouchscreenDevice(
      2, ui::InputDeviceType::INPUT_DEVICE_EXTERNAL, gfx::Size(1024, 768)));

  test::ScopedSetInternalDisplayId set_internal(display_manager(),
                                                displays_[0].id());
  AssociateTouchscreens(&displays_, devices, display_manager());

  EXPECT_EQ(0u, displays_[0].TouchDevicesCount());
  EXPECT_EQ(1u, displays_[1].TouchDevicesCount());
  EXPECT_TRUE(displays_[1].HasTouchDevice(ToIdentifier(devices[0])));
  EXPECT_EQ(0u, displays_[2].TouchDevicesCount());
  EXPECT_EQ(1u, displays_[3].TouchDevicesCount());
  EXPECT_TRUE(displays_[3].HasTouchDevice(ToIdentifier(devices[1])));
}

TEST_F(TouchscreenUtilTest, MapToCorrectDisplaySize) {
  std::vector<ui::TouchscreenDevice> devices;
  devices.push_back(CreateTouchscreenDevice(
      2, ui::InputDeviceType::INPUT_DEVICE_EXTERNAL, gfx::Size(1024, 768)));

  test::ScopedSetInternalDisplayId set_internal(display_manager(),
                                                displays_[0].id());
  AssociateTouchscreens(&displays_, devices, display_manager());

  EXPECT_EQ(0u, displays_[0].TouchDevicesCount());
  EXPECT_EQ(0u, displays_[1].TouchDevicesCount());
  EXPECT_EQ(0u, displays_[2].TouchDevicesCount());
  EXPECT_EQ(1u, displays_[3].TouchDevicesCount());
  EXPECT_TRUE(displays_[3].HasTouchDevice(ToIdentifier(devices[0])));
}

TEST_F(TouchscreenUtilTest, MapWhenSizeDiffersByOne) {
  std::vector<ui::TouchscreenDevice> devices;
  devices.push_back(CreateTouchscreenDevice(
      1, ui::InputDeviceType::INPUT_DEVICE_EXTERNAL, gfx::Size(801, 600)));
  devices.push_back(CreateTouchscreenDevice(
      2, ui::InputDeviceType::INPUT_DEVICE_EXTERNAL, gfx::Size(1023, 768)));

  test::ScopedSetInternalDisplayId set_internal(display_manager(),
                                                displays_[0].id());
  AssociateTouchscreens(&displays_, devices, display_manager());

  EXPECT_EQ(0u, displays_[0].TouchDevicesCount());
  EXPECT_EQ(1u, displays_[1].TouchDevicesCount());
  EXPECT_TRUE(displays_[1].HasTouchDevice(ToIdentifier(devices[0])));
  EXPECT_EQ(0u, displays_[2].TouchDevicesCount());
  EXPECT_EQ(1u, displays_[3].TouchDevicesCount());
  EXPECT_TRUE(displays_[3].HasTouchDevice(ToIdentifier(devices[1])));
}

TEST_F(TouchscreenUtilTest, MapWhenSizesDoNotMatch) {
  std::vector<ui::TouchscreenDevice> devices;
  devices.push_back(CreateTouchscreenDevice(
      1, ui::InputDeviceType::INPUT_DEVICE_EXTERNAL, gfx::Size(1022, 768)));
  devices.push_back(CreateTouchscreenDevice(
      2, ui::InputDeviceType::INPUT_DEVICE_EXTERNAL, gfx::Size(802, 600)));

  DisplayInfoList displays;
  displays.push_back(displays_[0]);
  displays.push_back(displays_[1]);

  test::ScopedSetInternalDisplayId set_internal(display_manager(),
                                                displays[0].id());
  AssociateTouchscreens(&displays, devices, display_manager());

  EXPECT_EQ(0u, displays[0].TouchDevicesCount());
  EXPECT_EQ(2u, displays[1].TouchDevicesCount());
  EXPECT_TRUE(displays[1].HasTouchDevice(ToIdentifier(devices[0])));
  EXPECT_TRUE(displays[1].HasTouchDevice(ToIdentifier(devices[1])));
}

TEST_F(TouchscreenUtilTest, MapInternalTouchscreen) {
  std::vector<ui::TouchscreenDevice> devices;
  devices.push_back(CreateTouchscreenDevice(
      1, ui::InputDeviceType::INPUT_DEVICE_EXTERNAL, gfx::Size(1920, 1080)));
  devices.push_back(CreateTouchscreenDevice(
      2, ui::InputDeviceType::INPUT_DEVICE_INTERNAL, gfx::Size(9999, 888)));
  devices.push_back(CreateTouchscreenDevice(
      3, ui::InputDeviceType::INPUT_DEVICE_EXTERNAL, gfx::Size(1234, 2345)));

  DisplayInfoList displays;
  displays.push_back(displays_[0]);
  displays.push_back(displays_[1]);

  test::ScopedSetInternalDisplayId set_internal(display_manager(),
                                                displays[0].id());
  AssociateTouchscreens(&displays, devices, display_manager());

  // Internal touchscreen is always mapped to internal display.
  EXPECT_EQ(2u, displays[0].TouchDevicesCount());
  EXPECT_TRUE(displays[0].HasTouchDevice(ToIdentifier(devices[1])));
  EXPECT_TRUE(displays[0].HasTouchDevice(ToIdentifier(devices[0])));
  EXPECT_EQ(1u, displays[1].TouchDevicesCount());
  EXPECT_TRUE(displays[1].HasTouchDevice(ToIdentifier(devices[2])));
}

TEST_F(TouchscreenUtilTest, MultipleInternal) {
  std::vector<ui::TouchscreenDevice> devices;
  devices.push_back(CreateTouchscreenDevice(
      1, ui::InputDeviceType::INPUT_DEVICE_INTERNAL, gfx::Size(1920, 1080)));
  devices.push_back(CreateTouchscreenDevice(
      2, ui::InputDeviceType::INPUT_DEVICE_INTERNAL, gfx::Size(1920, 1080)));

  test::ScopedSetInternalDisplayId set_internal(display_manager(),
                                                displays_[0].id());
  AssociateTouchscreens(&displays_, devices, display_manager());

  EXPECT_EQ(2u, displays_[0].TouchDevicesCount());
  EXPECT_EQ(0u, displays_[1].TouchDevicesCount());
  EXPECT_EQ(0u, displays_[2].TouchDevicesCount());
  EXPECT_EQ(0u, displays_[3].TouchDevicesCount());
}

TEST_F(TouchscreenUtilTest, MultipleInternalAndExternal) {
  std::vector<ui::TouchscreenDevice> devices;
  devices.push_back(CreateTouchscreenDevice(
      1, ui::InputDeviceType::INPUT_DEVICE_INTERNAL, gfx::Size(1920, 1080)));
  devices.push_back(CreateTouchscreenDevice(
      2, ui::InputDeviceType::INPUT_DEVICE_INTERNAL, gfx::Size(1920, 1080)));
  devices.push_back(CreateTouchscreenDevice(
      3, ui::InputDeviceType::INPUT_DEVICE_EXTERNAL, gfx::Size(1024, 768)));

  test::ScopedSetInternalDisplayId set_internal(display_manager(),
                                                displays_[0].id());
  AssociateTouchscreens(&displays_, devices, display_manager());

  EXPECT_EQ(2u, displays_[0].TouchDevicesCount());
  EXPECT_TRUE(displays_[0].HasTouchDevice(ToIdentifier(devices[0])));
  EXPECT_TRUE(displays_[0].HasTouchDevice(ToIdentifier(devices[1])));
  EXPECT_EQ(0u, displays_[1].TouchDevicesCount());
  EXPECT_EQ(0u, displays_[2].TouchDevicesCount());
  EXPECT_EQ(1u, displays_[3].TouchDevicesCount());
  EXPECT_TRUE(displays_[3].HasTouchDevice(ToIdentifier(devices[2])));
}

// crbug.com/515201
TEST_F(TouchscreenUtilTest, TestWithNoInternalDisplay) {
  std::vector<ui::TouchscreenDevice> devices;
  devices.push_back(CreateTouchscreenDevice(
      1, ui::InputDeviceType::INPUT_DEVICE_EXTERNAL, gfx::Size(1920, 1080)));
  devices.push_back(CreateTouchscreenDevice(
      2, ui::InputDeviceType::INPUT_DEVICE_INTERNAL, gfx::Size(9999, 888)));

  // Internal touchscreen should not be associated with any display
  AssociateTouchscreens(&displays_, devices, display_manager());

  EXPECT_EQ(1u, displays_[0].TouchDevicesCount());
  EXPECT_TRUE(displays_[0].HasTouchDevice(ToIdentifier(devices[0])));
  EXPECT_EQ(0u, displays_[1].TouchDevicesCount());
  EXPECT_EQ(0u, displays_[2].TouchDevicesCount());
  EXPECT_EQ(0u, displays_[3].TouchDevicesCount());
}

TEST_F(TouchscreenUtilTest, UnmatchedDevicesAreAssociatedWithInternalDisplay) {
  std::vector<ui::TouchscreenDevice> devices;
  devices.push_back(CreateTouchscreenDevice(
      1, ui::InputDeviceType::INPUT_DEVICE_EXTERNAL, gfx::Size(1920, 1080)));
  devices.push_back(CreateTouchscreenDevice(
      2, ui::InputDeviceType::INPUT_DEVICE_INTERNAL, gfx::Size(9999, 888)));
  devices.push_back(CreateTouchscreenDevice(
      3, ui::InputDeviceType::INPUT_DEVICE_EXTERNAL, gfx::Size(800, 600)));
  devices.push_back(CreateTouchscreenDevice(
      4, ui::InputDeviceType::INPUT_DEVICE_EXTERNAL, gfx::Size(234, 456)));
  devices.push_back(CreateTouchscreenDevice(
      5, ui::InputDeviceType::INPUT_DEVICE_EXTERNAL, gfx::Size(345, 678)));

  test::ScopedSetInternalDisplayId set_internal(display_manager(),
                                                displays_[0].id());

  AssociateTouchscreens(&displays_, devices, display_manager());

  // Internal display and the internal touch device are associated.
  EXPECT_TRUE(displays_[0].HasTouchDevice(ToIdentifier(devices[1])));

  // Since they have the same size, they are matched.
  EXPECT_TRUE(displays_[1].HasTouchDevice(ToIdentifier(devices[2])));

  // All the remaining touch devices should be associated with the internal
  // display.
  EXPECT_EQ(displays_[0].TouchDevicesCount(), 4UL);
}

TEST_F(TouchscreenUtilTest, AllDevicesAreAssociated) {
  std::vector<ui::TouchscreenDevice> devices;
  devices.push_back(CreateTouchscreenDevice(
      1, ui::InputDeviceType::INPUT_DEVICE_EXTERNAL, gfx::Size(1920, 1080)));
  devices.push_back(CreateTouchscreenDevice(
      2, ui::InputDeviceType::INPUT_DEVICE_INTERNAL, gfx::Size(9999, 888)));
  devices.push_back(CreateTouchscreenDevice(
      3, ui::InputDeviceType::INPUT_DEVICE_EXTERNAL, gfx::Size(800, 600)));
  devices.push_back(CreateTouchscreenDevice(
      4, ui::InputDeviceType::INPUT_DEVICE_EXTERNAL, gfx::Size(234, 456)));
  devices.push_back(CreateTouchscreenDevice(
      5, ui::InputDeviceType::INPUT_DEVICE_EXTERNAL, gfx::Size(345, 678)));

  AssociateTouchscreens(&displays_, devices, display_manager());

  // One less, since the internal device will not be associated to anything if
  // there are no internal displays available.
  const std::size_t expected_devices_to_be_associated = devices.size() - 1UL;
  std::size_t devices_associated = 0UL;

  for (std::size_t i = 0; i < displays_.size(); i++)
    devices_associated += displays_[i].TouchDevicesCount();

  EXPECT_EQ(devices_associated, expected_devices_to_be_associated);
}

TEST_F(TouchscreenUtilTest, AssociateBasedOnStoredPreference) {
  display::test::DisplayManagerTestApi(display_manager())
      .UpdateDisplay("100x100,101+0-100x100,205+0-100x100,315+0-100x100");

  DisplayIdList active_id_list = display_manager()->GetCurrentDisplayIdList();
  const int kTouchDeviceCount = 5;
  std::map<uint32_t, int64_t> expected_mapping;
  const int64_t kInternalDisplayId = displays_[0].id();
  test::ScopedSetInternalDisplayId set_internal(display_manager(),
                                                kInternalDisplayId);

  std::vector<ui::TouchscreenDevice> devices;
  devices.push_back(CreateTouchscreenDevice(
      0, ui::InputDeviceType::INPUT_DEVICE_INTERNAL, gfx::Size(256, 256)));

  // The internal touch device should be matching with the internal display id.
  expected_mapping[TouchCalibrationData::GenerateTouchDeviceIdentifier(
      devices[0])] = kInternalDisplayId;

  for (int i = 1; i < kTouchDeviceCount; ++i) {
    devices.push_back(
        CreateTouchscreenDevice(i, ui::InputDeviceType::INPUT_DEVICE_EXTERNAL,
                                gfx::Size(256 * i, 256 * i)));
  }

  // Populate internally managed display info, as if they were populated by
  // display preferences. We associate touch devices with displays (except for
  // the internal display and internal touch device).
  uint32_t touch_device_identifier;
  std::size_t display_id_index = 0;
  for (int i = kTouchDeviceCount - 1;
       i > 1 && display_id_index < active_id_list.size();
       i--, display_id_index++) {
    if (active_id_list[display_id_index] == kInternalDisplayId) {
      touch_device_identifier =
          TouchCalibrationData::GenerateTouchDeviceIdentifier(devices[1]);
      const int64_t kNonInternalDisplayId =
          active_id_list[active_id_list.size() - display_id_index - 1];
      display::test::DisplayManagerTestApi(display_manager())
          .AddTouchDevice(kNonInternalDisplayId, touch_device_identifier);
      expected_mapping[touch_device_identifier] = kNonInternalDisplayId;

      display_id_index++;
    }

    touch_device_identifier =
        TouchCalibrationData::GenerateTouchDeviceIdentifier(devices[i]);
    int64_t display_id = active_id_list[display_id_index];
    display::test::DisplayManagerTestApi(display_manager())
        .AddTouchDevice(display_id, touch_device_identifier);
    expected_mapping[touch_device_identifier] = display_id;
  }

  AssociateTouchscreens(&displays_, devices, display_manager());

  for (const auto& touch_device : devices) {
    touch_device_identifier =
        TouchCalibrationData::GenerateTouchDeviceIdentifier(touch_device);
    for (const auto& display : displays_) {
      if (display.HasTouchDevice(touch_device_identifier))
        EXPECT_TRUE(expected_mapping[touch_device_identifier] == display.id());
    }
  }
}

}  // namespace display
