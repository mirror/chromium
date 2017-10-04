// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/extensions/chooser_dialog_view.h"
#include "chrome/browser/ui/views/permission_bubble/chooser_bubble_ui.h"

#include <string>

#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/chooser_controller/fake_bluetooth_chooser_controller.h"
#include "chrome/browser/chooser_controller/fake_usb_chooser_controller.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/test/test_browser_dialog.h"
#include "components/constrained_window/constrained_window_views.h"

namespace {

void ShowChooserBubble(Browser* browser, ChooserController* controller) {
  auto* bubble = new ChooserBubbleUi(
      browser, std::unique_ptr<ChooserController>(controller));
  bubble->Show(nullptr);
}

void ShowChooserModal(Browser* browser, ChooserController* controller) {
  auto* web_contents = browser->tab_strip_model()->GetActiveWebContents();
  constrained_window::ShowWebModalDialogViews(
      new ChooserDialogView(std::unique_ptr<ChooserController>(controller)),
      web_contents);
}

void ShowChooser(std::string test_name,
                 Browser* browser,
                 ChooserController* controller) {
  if (base::EndsWith(test_name, "modal", base::CompareCase::SENSITIVE))
    ShowChooserModal(browser, controller);
  else
    ShowChooserBubble(browser, controller);
}

}  // namespace

// Invokes a bubble allowing user to select a USB device for a web page. See
// test_browser_dialog.h.
class UsbChooserBrowserTest : public DialogBrowserTest {
 public:
  UsbChooserBrowserTest() : option_count_(0) {}

  // DialogBrowserTest:
  void ShowDialog(const std::string& name) override {
    auto* controller = new FakeUsbChooserController(option_count_);
    ShowChooser(name, browser(), controller);
  }

  void set_option_count(int option_count) { option_count_ = option_count; }

 private:
  int option_count_;

  DISALLOW_COPY_AND_ASSIGN(UsbChooserBrowserTest);
};

IN_PROC_BROWSER_TEST_F(UsbChooserBrowserTest, InvokeDialog_noDevices_bubble) {
  RunDialog();
}

IN_PROC_BROWSER_TEST_F(UsbChooserBrowserTest, InvokeDialog_noDevices_modal) {
  RunDialog();
}

IN_PROC_BROWSER_TEST_F(UsbChooserBrowserTest, InvokeDialog_withDevices_bubble) {
  set_option_count(5);
  RunDialog();
}

IN_PROC_BROWSER_TEST_F(UsbChooserBrowserTest, InvokeDialog_withDevices_modal) {
  set_option_count(5);
  RunDialog();
}

// Invokes a bubble allowing user to select a Bluetooth device for a web page.
// See test_browser_dialog.h.
class BluetoothChooserBrowserTest : public DialogBrowserTest {
 public:
  BluetoothChooserBrowserTest()
      : status_(FakeBluetoothChooserController::UNAVAILABLE) {}

  // DialogBrowserTest:
  void ShowDialog(const std::string& name) override {
    auto* controller = new FakeBluetoothChooserController(std::move(devices_));
    ShowChooser(name, browser(), controller);
    controller->SetBluetoothStatus(status_);
  }

  void set_status(FakeBluetoothChooserController::Status status) {
    status_ = status;
  }
  void add_device(FakeBluetoothChooserController::FakeDevice device) {
    devices_.push_back(device);
  }

 private:
  FakeBluetoothChooserController::Status status_;
  std::vector<FakeBluetoothChooserController::FakeDevice> devices_;

  DISALLOW_COPY_AND_ASSIGN(BluetoothChooserBrowserTest);
};

IN_PROC_BROWSER_TEST_F(BluetoothChooserBrowserTest,
                       InvokeDialog_unavailable_bubble) {
  RunDialog();
}

IN_PROC_BROWSER_TEST_F(BluetoothChooserBrowserTest,
                       InvokeDialog_unavailable_modal) {
  RunDialog();
}

IN_PROC_BROWSER_TEST_F(BluetoothChooserBrowserTest,
                       InvokeDialog_noDevices_bubble) {
  set_status(FakeBluetoothChooserController::IDLE);
  RunDialog();
}

IN_PROC_BROWSER_TEST_F(BluetoothChooserBrowserTest,
                       InvokeDialog_noDevices_modal) {
  set_status(FakeBluetoothChooserController::IDLE);
  RunDialog();
}

IN_PROC_BROWSER_TEST_F(BluetoothChooserBrowserTest,
                       InvokeDialog_scanning_bubble) {
  set_status(FakeBluetoothChooserController::SCANNING);
  RunDialog();
}

IN_PROC_BROWSER_TEST_F(BluetoothChooserBrowserTest,
                       InvokeDialog_scanning_modal) {
  set_status(FakeBluetoothChooserController::SCANNING);
  RunDialog();
}

IN_PROC_BROWSER_TEST_F(BluetoothChooserBrowserTest,
                       InvokeDialog_scanningWithDevices_bubble) {
  set_status(FakeBluetoothChooserController::SCANNING);
  add_device(FakeBluetoothChooserController::FakeDevice(false, false, 1));
  add_device(FakeBluetoothChooserController::FakeDevice(false, false, 2));
  add_device(FakeBluetoothChooserController::FakeDevice(false, false, 3));
  add_device(FakeBluetoothChooserController::FakeDevice(false, false, 4));
  RunDialog();
}

IN_PROC_BROWSER_TEST_F(BluetoothChooserBrowserTest,
                       InvokeDialog_scanningWithDevices_modal) {
  set_status(FakeBluetoothChooserController::SCANNING);
  add_device(FakeBluetoothChooserController::FakeDevice(false, false, 1));
  add_device(FakeBluetoothChooserController::FakeDevice(false, false, 2));
  add_device(FakeBluetoothChooserController::FakeDevice(false, false, 3));
  add_device(FakeBluetoothChooserController::FakeDevice(false, false, 4));
  RunDialog();
}

IN_PROC_BROWSER_TEST_F(BluetoothChooserBrowserTest,
                       InvokeDialog_connected_bubble) {
  set_status(FakeBluetoothChooserController::IDLE);
  add_device(FakeBluetoothChooserController::FakeDevice(true, false, 4));
  RunDialog();
}

IN_PROC_BROWSER_TEST_F(BluetoothChooserBrowserTest,
                       InvokeDialog_connected_modal) {
  set_status(FakeBluetoothChooserController::IDLE);
  add_device(FakeBluetoothChooserController::FakeDevice(true, false, 4));
  RunDialog();
}

IN_PROC_BROWSER_TEST_F(BluetoothChooserBrowserTest,
                       InvokeDialog_paired_bubble) {
  set_status(FakeBluetoothChooserController::IDLE);
  add_device(FakeBluetoothChooserController::FakeDevice(false, true, 4));
  RunDialog();
}

IN_PROC_BROWSER_TEST_F(BluetoothChooserBrowserTest, InvokeDialog_paired_modal) {
  set_status(FakeBluetoothChooserController::IDLE);
  add_device(FakeBluetoothChooserController::FakeDevice(false, true, 4));
  RunDialog();
}
