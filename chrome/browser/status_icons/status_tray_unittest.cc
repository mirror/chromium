// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/status_icons/status_tray.h"

#include "base/compiler_specific.h"
#include "base/memory/ptr_util.h"
#include "chrome/browser/status_icons/status_icon.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/gfx/image/image_skia.h"
#include "ui/gfx/image/image_unittest_util.h"
#include "ui/message_center/public/cpp/notifier_id.h"

namespace {

class MockStatusIcon : public StatusIcon {
  void SetImage(const gfx::ImageSkia& image) override {}
  void SetToolTip(const base::string16& tool_tip) override {}
  void DisplayBalloon(const gfx::ImageSkia& icon,
                      const base::string16& title,
                      const base::string16& contents,
                      const message_center::NotifierId& notifier_id) override {}
  void UpdatePlatformContextMenu(StatusIconMenuModel* menu) override {}
};

class TestStatusTray : public StatusTray {
 public:
  std::unique_ptr<StatusIcon> CreatePlatformStatusIcon(
      StatusIconType type,
      const gfx::ImageSkia& image,
      const base::string16& tool_tip) override {
    return base::MakeUnique<MockStatusIcon>();
  }

  const StatusIcons& GetStatusIconsForTest() const { return status_icons(); }
};

StatusIcon* CreateStatusIcon(StatusTray* tray) {
  // Just create a dummy icon image; the actual image is irrelevant.
  return tray->CreateStatusIcon(StatusTray::OTHER_ICON,
                                gfx::test::CreateImageSkia(16, 16),
                                base::string16());
}

}  // namespace

TEST(StatusTrayTest, Create) {
  // Check for creation and leaks.
  TestStatusTray tray;
  CreateStatusIcon(&tray);
  EXPECT_EQ(1U, tray.GetStatusIconsForTest().size());
}

// Make sure that removing an icon removes it from the list.
TEST(StatusTrayTest, CreateRemove) {
  TestStatusTray tray;
  StatusIcon* icon = CreateStatusIcon(&tray);
  EXPECT_EQ(1U, tray.GetStatusIconsForTest().size());
  tray.RemoveStatusIcon(icon);
  EXPECT_EQ(0U, tray.GetStatusIconsForTest().size());
}
