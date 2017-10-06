// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/device_chooser_content_view.h"

#include <memory>

#include "base/macros.h"
#include "chrome/browser/chooser_controller/fake_bluetooth_chooser_controller.h"
#include "chrome/grit/generated_resources.h"
#include "chrome/test/views/chrome_views_test_base.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/models/list_selection_model.h"
#include "ui/views/controls/link.h"
#include "ui/views/controls/styled_label.h"
#include "ui/views/controls/table/table_view.h"
#include "ui/views/controls/table/table_view_observer.h"
#include "ui/views/controls/throbber.h"

namespace {

class MockTableViewObserver : public views::TableViewObserver {
 public:
  // views::TableViewObserver:
  MOCK_METHOD0(OnSelectionChanged, void());
};

base::string16 GetPairedText(const base::string16& device_name) {
  return l10n_util::GetStringFUTF16(
      IDS_DEVICE_CHOOSER_DEVICE_NAME_AND_PAIRED_STATUS_TEXT, device_name);
}

}  // namespace

class DeviceChooserContentViewTest : public ChromeViewsTestBase {
 public:
  DeviceChooserContentViewTest() {}

  void SetUp() override {
    ChromeViewsTestBase::SetUp();
    table_observer_ = base::MakeUnique<MockTableViewObserver>();
    controller_ = new FakeBluetoothChooserController();
    content_view_ = base::MakeUnique<DeviceChooserContentView>(
        table_observer_.get(), std::unique_ptr<ChooserController>(controller_));
    controller_->SetBluetoothStatus(FakeBluetoothChooserController::IDLE);
  }

  FakeBluetoothChooserController& controller() { return *controller_; }
  MockTableViewObserver& table_observer() { return *table_observer_; }
  DeviceChooserContentView& content_view() { return *content_view_; }

  views::TableView& table_view() { return *content_view().table_view_; }

  ui::TableModel& table_model() { return *table_view().model(); }

  views::Throbber& throbber() { return *content_view().throbber_; }

  views::StyledLabel& help_link() {
    return *content_view().turn_adapter_off_help_;
  }

  views::StyledLabel* footnote_link() {
    return content_view().footnote_link_.get();
  }

  void AddDevice() {
    controller().AddDevice(FakeBluetoothChooserController::FakeDevice(
        false, false, FakeBluetoothChooserController::kSignalStrengthLevel1));
  }

  void AddPairedDevice() {
    controller().AddDevice(FakeBluetoothChooserController::FakeDevice(
        true, true, FakeBluetoothChooserController::kSignalStrengthUnknown));
  }

  base::string16 GetDeviceNameAtRow(size_t row_index) {
    return controller().GetOption(row_index);
  }

  bool IsDeviceSelected() { return table_view().selection_model().size() > 0; }

  void AssertNoDevices() {
    // "No devices found." is displayed in the table, so there's exactly 1 row.
    EXPECT_EQ(1, table_view().RowCount());
    EXPECT_EQ(l10n_util::GetStringUTF16(
                  IDS_BLUETOOTH_DEVICE_CHOOSER_NO_DEVICES_FOUND_PROMPT),
              table_model().GetText(0, 0));
    // The table should be disabled since there are no (real) options.
    EXPECT_FALSE(table_view().enabled());
    EXPECT_FALSE(IsDeviceSelected());
  }

 private:
  std::unique_ptr<MockTableViewObserver> table_observer_;
  FakeBluetoothChooserController* controller_ = nullptr;
  std::unique_ptr<DeviceChooserContentView> content_view_;

  DISALLOW_COPY_AND_ASSIGN(DeviceChooserContentViewTest);
};

TEST_F(DeviceChooserContentViewTest, InitialState) {
  EXPECT_CALL(table_observer(), OnSelectionChanged()).Times(0);

  EXPECT_TRUE(table_view().visible());
  AssertNoDevices();
  EXPECT_FALSE(throbber().visible());
  EXPECT_FALSE(help_link().visible());
  EXPECT_EQ(content_view().help_and_re_scan_text_, footnote_link()->text());
}

TEST_F(DeviceChooserContentViewTest, AddOption) {
  EXPECT_CALL(table_observer(), OnSelectionChanged()).Times(0);
  AddPairedDevice();

  EXPECT_EQ(1, table_view().RowCount());
  EXPECT_EQ(GetPairedText(GetDeviceNameAtRow(0)), table_model().GetText(0, 0));
  // The table should be enabled now that there's an option.
  EXPECT_TRUE(table_view().enabled());
  EXPECT_FALSE(IsDeviceSelected());

  AddDevice();
  EXPECT_EQ(2, table_view().RowCount());
  EXPECT_EQ(GetDeviceNameAtRow(1), table_model().GetText(1, 0));
  EXPECT_TRUE(table_view().enabled());
  EXPECT_FALSE(IsDeviceSelected());
}

TEST_F(DeviceChooserContentViewTest, RemoveOption) {
  // Called from TableView::OnItemsRemoved().
  EXPECT_CALL(table_observer(), OnSelectionChanged()).Times(3);
  AddPairedDevice();
  AddDevice();
  AddDevice();

  controller().RemoveDevice(1);
  EXPECT_EQ(2, table_view().RowCount());
  EXPECT_EQ(GetPairedText(GetDeviceNameAtRow(0)), table_model().GetText(0, 0));
  EXPECT_EQ(GetDeviceNameAtRow(1), table_model().GetText(1, 0));
  EXPECT_TRUE(table_view().enabled());
  EXPECT_FALSE(IsDeviceSelected());

  // Remove everything.
  controller().RemoveDevice(1);
  controller().RemoveDevice(0);
  // Should be back to the initial state.
  AssertNoDevices();
}

TEST_F(DeviceChooserContentViewTest, UpdateOption) {
  EXPECT_CALL(table_observer(), OnSelectionChanged()).Times(0);
  AddPairedDevice();
  AddDevice();
  AddDevice();

  controller().UpdateDevice(
      1,
      FakeBluetoothChooserController::FakeDevice(
          true, true, FakeBluetoothChooserController::kSignalStrengthUnknown));
  EXPECT_EQ(3, table_view().RowCount());
  EXPECT_EQ(GetPairedText(GetDeviceNameAtRow(1)), table_model().GetText(1, 0));
  EXPECT_FALSE(IsDeviceSelected());
}

TEST_F(DeviceChooserContentViewTest, SelectAndDeselectAnOption) {
  EXPECT_CALL(table_observer(), OnSelectionChanged()).Times(2);
  AddPairedDevice();
  AddDevice();

  table_view().Select(0);
  EXPECT_TRUE(IsDeviceSelected());
  EXPECT_EQ(0, table_view().FirstSelectedRow());

  table_view().Select(-1);
  EXPECT_FALSE(IsDeviceSelected());
}

TEST_F(DeviceChooserContentViewTest, SelectAnOptionAndRemoveAnotherOption) {
  EXPECT_CALL(table_observer(), OnSelectionChanged()).Times(2);
  AddPairedDevice();
  AddDevice();

  table_view().Select(1);
  EXPECT_EQ(1, table_view().FirstSelectedRow());

  controller().RemoveDevice(0);
  EXPECT_EQ(1, table_view().RowCount());
  EXPECT_TRUE(IsDeviceSelected());
  // Since option 0 was removed, the originally selected option 1 becomes
  // option 0.
  EXPECT_EQ(0, table_view().FirstSelectedRow());
}

TEST_F(DeviceChooserContentViewTest, SelectAnOptionAndRemoveTheSelectedOption) {
  EXPECT_CALL(table_observer(), OnSelectionChanged()).Times(2);
  AddPairedDevice();
  AddDevice();

  table_view().Select(1);
  controller().RemoveDevice(1);
  EXPECT_FALSE(IsDeviceSelected());
}

TEST_F(DeviceChooserContentViewTest, TurnBluetoothOffAndOn) {
  controller().SetBluetoothStatus(FakeBluetoothChooserController::UNAVAILABLE);

  EXPECT_FALSE(table_view().visible());
  EXPECT_TRUE(help_link().visible());
  EXPECT_EQ(content_view().help_text_, footnote_link()->text());

  controller().SetBluetoothStatus(FakeBluetoothChooserController::IDLE);
  AssertNoDevices();
}

TEST_F(DeviceChooserContentViewTest, FindNoDevices) {
  controller().SetBluetoothStatus(FakeBluetoothChooserController::SCANNING);
  EXPECT_FALSE(table_view().visible());
  EXPECT_TRUE(throbber().visible());
  EXPECT_EQ(content_view().help_and_scanning_text_, footnote_link()->text());
}

TEST_F(DeviceChooserContentViewTest, FindOneDevice) {
  controller().SetBluetoothStatus(FakeBluetoothChooserController::SCANNING);
  AddDevice();
  EXPECT_TRUE(table_view().visible());
  EXPECT_TRUE(table_view().enabled());
  EXPECT_EQ(1, table_view().RowCount());
  EXPECT_FALSE(IsDeviceSelected());
}

TEST_F(DeviceChooserContentViewTest, ClickAdapterOffHelpLink) {
  EXPECT_CALL(controller(), OpenAdapterOffHelpUrl()).Times(1);
  help_link().LinkClicked(nullptr, 0);
}

TEST_F(DeviceChooserContentViewTest, ClickRescanLink) {
  EXPECT_CALL(controller(), RefreshOptions()).Times(1);
  content_view().StyledLabelLinkClicked(footnote_link(),
                                        content_view().re_scan_text_range_, 0);
}

TEST_F(DeviceChooserContentViewTest, ClickGetHelpLink) {
  EXPECT_CALL(controller(), OpenHelpCenterUrl()).Times(1);
  content_view().StyledLabelLinkClicked(footnote_link(),
                                        content_view().help_text_range_, 0);
}
