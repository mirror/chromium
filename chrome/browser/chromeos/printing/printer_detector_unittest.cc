// Copyright 2017 the Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "chrome/browser/chromeos/printing/printer_detector.h"

#include <vector>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/memory/ptr_util.h"
#include "base/sequenced_task_runner.h"
#include "base/test/scoped_task_environment.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "chrome/browser/chromeos/printing/usb_printer_detector.h"
#include "chrome/browser/chromeos/printing/zeroconf_printer_detector.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/test/base/testing_profile.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "content/public/test/test_utils.h"
#include "device/usb/usb_service.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace chromeos {
namespace {

// Fake PrinterDetector::Observer which is used to test the PrinterDetector
// class.
class FakePrinterDetectorObserver : public PrinterDetector::Observer {
 public:
  FakePrinterDetectorObserver() : on_printers_found_calls_(0) {}

  void OnPrintersFound(
      const std::vector<PrinterDetector::DetectedPrinter>& printers) override {
    ++on_printers_found_calls_;
  }

  void OnPrinterScanComplete() override {}

  uint32_t OnPrintersFoundCalls() const { return on_printers_found_calls_; }

 private:
  uint32_t on_printers_found_calls_;
};

class PrinterDetectorTest : public testing::Test {
 public:
  PrinterDetectorTest() {
    usb_service_ = device::UsbService::Create();
    usb_printer_detector_ =
        UsbPrinterDetector::CreateForTesting(usb_service_.get());
    zeroconf_printer_detector_ = ZeroconfPrinterDetector::Create(&profile_);
  }

  ~PrinterDetectorTest() override {}

 protected:
  content::TestBrowserThreadBundle test_browser_thread_bundle_;
  TestingProfile profile_;

  std::unique_ptr<device::UsbService> usb_service_;
  std::unique_ptr<PrinterDetector> usb_printer_detector_;
  std::unique_ptr<PrinterDetector> zeroconf_printer_detector_;
};

TEST_F(PrinterDetectorTest, UsbPrinterDetectorStartObservers) {
  FakePrinterDetectorObserver fake_printer_detector_observer;
  usb_printer_detector_->AddObserver(&fake_printer_detector_observer);
  EXPECT_EQ(fake_printer_detector_observer.OnPrintersFoundCalls(), 0U);

  usb_printer_detector_->StartObservers();
  content::RunAllPendingInMessageLoop();

  EXPECT_EQ(fake_printer_detector_observer.OnPrintersFoundCalls(), 1U);

  usb_printer_detector_->RemoveObserver(&fake_printer_detector_observer);
}

TEST_F(PrinterDetectorTest, ZeroconfPrinterDetectorStartObservers) {
  FakePrinterDetectorObserver fake_printer_detector_observer;
  zeroconf_printer_detector_->AddObserver(&fake_printer_detector_observer);
  EXPECT_EQ(fake_printer_detector_observer.OnPrintersFoundCalls(), 0U);

  zeroconf_printer_detector_->StartObservers();
  content::RunAllPendingInMessageLoop();

  EXPECT_EQ(fake_printer_detector_observer.OnPrintersFoundCalls(), 1U);

  zeroconf_printer_detector_->RemoveObserver(&fake_printer_detector_observer);
}
}  // namespace
}  // namespace chromeos
