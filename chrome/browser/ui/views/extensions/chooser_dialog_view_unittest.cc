// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/extensions/chooser_dialog_view.h"

#include <memory>

#include "base/macros.h"
#include "chrome/browser/chooser_controller/fake_bluetooth_chooser_controller.h"
#include "chrome/browser/ui/views/device_chooser_content_view.h"
#include "chrome/test/views/chrome_views_test_base.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "ui/views/controls/table/table_view.h"
#include "ui/views/widget/widget.h"

class ChooserDialogViewTest : public ChromeViewsTestBase {
 public:
  ChooserDialogViewTest() {}

  void SetUp() override {
    ChromeViewsTestBase::SetUp();
    controller_ = new FakeBluetoothChooserController();
    dialog_ = new ChooserDialogView(
        std::unique_ptr<FakeBluetoothChooserController>(controller_));
    controller_->SetBluetoothStatus(FakeBluetoothChooserController::IDLE);

    widget_ = views::DialogDelegate::CreateDialogWidget(dialog_, GetContext(),
                                                        nullptr);
  }

  void TearDown() override {
    widget_->Close();
    ChromeViewsTestBase::TearDown();
  }

  ChooserDialogView& dialog() { return *dialog_; }
  FakeBluetoothChooserController& controller() { return *controller_; }

  views::TableView& table_view() {
    return *dialog().device_chooser_content_view_for_test()->table_view_;
  }

 private:
  ChooserDialogView* dialog_ = nullptr;
  FakeBluetoothChooserController* controller_ = nullptr;
  views::Widget* widget_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(ChooserDialogViewTest);
};

TEST_F(ChooserDialogViewTest, ButtonState) {
  // Cancel button is always enabled.
  EXPECT_TRUE(dialog().IsDialogButtonEnabled(ui::DIALOG_BUTTON_CANCEL));

  EXPECT_FALSE(dialog().IsDialogButtonEnabled(ui::DIALOG_BUTTON_OK));
  controller().AddDevice(FakeBluetoothChooserController::FakeDevice(
      false, false, FakeBluetoothChooserController::kSignalStrengthLevel1));
  EXPECT_FALSE(dialog().IsDialogButtonEnabled(ui::DIALOG_BUTTON_OK));
  table_view().Select(0);
  EXPECT_TRUE(dialog().IsDialogButtonEnabled(ui::DIALOG_BUTTON_OK));
}

TEST_F(ChooserDialogViewTest, Accept) {
  EXPECT_CALL(controller(), Select(testing::_)).Times(1);
  dialog().Accept();
}

TEST_F(ChooserDialogViewTest, Cancel) {
  EXPECT_CALL(controller(), Cancel()).Times(1);
  dialog().Cancel();
}

TEST_F(ChooserDialogViewTest, Close) {
  // Called from Widget::Close() in TearDown().
  EXPECT_CALL(controller(), Close()).Times(1);
}
