// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHOOSER_CONTROLLER_FAKE_USB_CHOOSER_CONTROLLER_H_
#define CHROME_BROWSER_CHOOSER_CONTROLLER_FAKE_USB_CHOOSER_CONTROLLER_H_

#include "base/macros.h"
#include "base/strings/string16.h"
#include "chrome/browser/chooser_controller/chooser_controller.h"

// A subclass of ChooserController that pretends to be a USB device chooser for
// testing. The result should be visually similar to the real version of the
// dialog for interactive tests.
class FakeUsbChooserController : public ChooserController {
 public:
  explicit FakeUsbChooserController(int option_count);

  // ChooserController:
  base::string16 GetNoOptionsText() const override;
  base::string16 GetOkButtonLabel() const override;
  size_t NumOptions() const override;
  base::string16 GetOption(size_t index) const override;
  void Select(const std::vector<size_t>& indices) override {}
  void Cancel() override {}
  void Close() override {}
  void OpenHelpCenterUrl() const override {}

 private:
  size_t option_count_ = 0;

  DISALLOW_COPY_AND_ASSIGN(FakeUsbChooserController);
};

#endif  // CHROME_BROWSER_CHOOSER_CONTROLLER_FAKE_USB_CHOOSER_CONTROLLER_H_
