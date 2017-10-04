// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chooser_controller/fake_bluetooth_chooser_controller.h"

#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/grit/generated_resources.h"
#include "ui/base/l10n/l10n_util.h"

FakeBluetoothChooserController::FakeBluetoothChooserController(
    std::vector<FakeDevice> devices)
    : ChooserController(nullptr, 0, 0), devices_(std::move(devices)) {
  set_title_for_testing(
      l10n_util::GetStringFUTF16(IDS_BLUETOOTH_DEVICE_CHOOSER_PROMPT_ORIGIN,
                                 base::ASCIIToUTF16("example.com")));
}

bool FakeBluetoothChooserController::ShouldShowIconBeforeText() const {
  return true;
}

base::string16 FakeBluetoothChooserController::GetNoOptionsText() const {
  return l10n_util::GetStringUTF16(
      IDS_BLUETOOTH_DEVICE_CHOOSER_NO_DEVICES_FOUND_PROMPT);
}

base::string16 FakeBluetoothChooserController::GetOkButtonLabel() const {
  return l10n_util::GetStringUTF16(
      IDS_BLUETOOTH_DEVICE_CHOOSER_PAIR_BUTTON_TEXT);
}

size_t FakeBluetoothChooserController::NumOptions() const {
  return devices_.size();
}

int FakeBluetoothChooserController::GetSignalStrengthLevel(size_t index) const {
  return devices_.at(index).signal_strength_;
}

base::string16 FakeBluetoothChooserController::GetOption(size_t index) const {
  return base::ASCIIToUTF16(base::StringPrintf("Device #%u", index));
}

bool FakeBluetoothChooserController::IsConnected(size_t index) const {
  return devices_.at(index).connected_;
}

bool FakeBluetoothChooserController::IsPaired(size_t index) const {
  return devices_.at(index).paired_;
}

base::string16 FakeBluetoothChooserController::GetStatus() const {
  switch (status_) {
    case UNAVAILABLE:
      return base::string16();
    case IDLE:
      return l10n_util::GetStringUTF16(IDS_BLUETOOTH_DEVICE_CHOOSER_RE_SCAN);
    case SCANNING:
      return l10n_util::GetStringUTF16(IDS_BLUETOOTH_DEVICE_CHOOSER_SCANNING);
  }
  NOTREACHED();
  return base::string16();
}

void FakeBluetoothChooserController::SetBluetoothStatus(Status status) {
  status_ = status;
  const bool available = status != UNAVAILABLE;
  view()->OnAdapterEnabledChanged(available);
  if (available)
    view()->OnRefreshStateChanged(status_ == SCANNING);
}
