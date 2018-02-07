// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#ifndef CONTENT_SHELL_BROWSER_LAYOUT_TEST_FAKE_BLUETOOTH_CHOOSER_H_
#define CONTENT_SHELL_BROWSER_LAYOUT_TEST_FAKE_BLUETOOTH_CHOOSER_H_

#include "base/macros.h"
#include "content/public/browser/bluetooth_chooser.h"
#include "content/shell/common/layout_test/fake_bluetooth_chooser.mojom.h"

namespace content {

// Implementation of FakeBluetoothChooser in
// src/content/shell/common/layout_test/fake_bluetooth_chooser.mojom
// with inheritance from BluetoothChooser in
// src/content/public/browser/bluetooth_chooser.h
// to provide a method of controlling the Bluetooth chooser during a test.
//
// The implementation details for FakeBluetoothChooser can be found in the Web
// Bluetooth Test Scanning design document.
// https://docs.google.com/document/d/1fwK4HqV41q1PuppL5By6NnlcDQefEu7BNeiOs1UMe04
//
// Not intended for direct use by clients.
class FakeBluetoothChooser : public mojom::FakeBluetoothChooser,
                             public BluetoothChooser {
 public:
  FakeBluetoothChooser(const EventHandler& event_handler);
  ~FakeBluetoothChooser() override;

  void WaitForEvents(uint32_t num_of_events,
                     WaitForEventsCallback callback) override;
  void SelectPeripheral(const std::string& peripheral_address,
                        SelectPeripheralCallback callback) override;
  void Cancel(CancelCallback callback) override;
  void Rescan(RescanCallback callback) override;

 private:
  EventHandler event_handler_;

  DISALLOW_COPY_AND_ASSIGN(FakeBluetoothChooser);
};

}  // namespace content

#endif  // CONTENT_SHELL_BROWSER_LAYOUT_TEST_FAKE_BLUETOOTH_CHOOSER_H_
