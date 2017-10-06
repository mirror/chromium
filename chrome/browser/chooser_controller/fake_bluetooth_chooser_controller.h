// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHOOSER_CONTROLLER_FAKE_BLUETOOTH_CHOOSER_CONTROLLER_H_
#define CHROME_BROWSER_CHOOSER_CONTROLLER_FAKE_BLUETOOTH_CHOOSER_CONTROLLER_H_

#include "base/macros.h"
#include "base/strings/string16.h"
#include "chrome/browser/chooser_controller/chooser_controller.h"
#include "testing/gmock/include/gmock/gmock.h"

// A subclass of ChooserController that pretends to be a Bluetooth device
// chooser for testing. The result should be visually similar to the real
// version of the dialog for interactive tests.
class FakeBluetoothChooserController : public ChooserController {
 public:
  enum Status {
    UNAVAILABLE,
    IDLE,
    SCANNING,
  };

  static constexpr int kSignalStrengthUnknown = -1;
  static constexpr int kSignalStrengthLevel0 = 0;
  static constexpr int kSignalStrengthLevel1 = 1;
  static constexpr int kSignalStrengthLevel2 = 2;
  static constexpr int kSignalStrengthLevel3 = 3;
  static constexpr int kSignalStrengthLevel4 = 4;

  struct FakeDevice {
    FakeDevice(bool connected, bool paired, int signal_strength);

    bool connected_;
    bool paired_;
    int signal_strength_;
  };

  FakeBluetoothChooserController();
  explicit FakeBluetoothChooserController(std::vector<FakeDevice> devices);

  // ChooserController:
  bool ShouldShowIconBeforeText() const override;
  base::string16 GetNoOptionsText() const override;
  base::string16 GetOkButtonLabel() const override;
  size_t NumOptions() const override;
  int GetSignalStrengthLevel(size_t index) const override;
  base::string16 GetOption(size_t index) const override;
  bool IsConnected(size_t index) const override;
  bool IsPaired(size_t index) const override;
  base::string16 GetStatus() const override;
  MOCK_METHOD0(RefreshOptions, void());
  MOCK_METHOD1(Select, void(const std::vector<size_t>& indices));
  MOCK_METHOD0(Cancel, void());
  MOCK_METHOD0(Close, void());
  MOCK_CONST_METHOD0(OpenHelpCenterUrl, void());
  MOCK_CONST_METHOD0(OpenAdapterOffHelpUrl, void());

  void SetBluetoothStatus(Status status);
  void AddDevice(FakeDevice device);
  void RemoveDevice(size_t index);
  void UpdateDevice(size_t index, FakeDevice new_device);

 private:
  Status status_ = UNAVAILABLE;
  std::vector<FakeDevice> devices_;

  DISALLOW_COPY_AND_ASSIGN(FakeBluetoothChooserController);
};

#endif  // CHROME_BROWSER_CHOOSER_CONTROLLER_FAKE_BLUETOOTH_CHOOSER_CONTROLLER_H_
