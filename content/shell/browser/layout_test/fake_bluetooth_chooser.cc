// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/shell/browser/layout_test/fake_bluetooth_chooser.h"

#include <string>
#include <utility>

#include "content/shell/common/layout_test/fake_bluetooth_chooser.mojom.h"
#include "mojo/public/cpp/bindings/strong_binding.h"

namespace content {

FakeBluetoothChooser::FakeBluetoothChooser(const EventHandler& event_handler)
    : event_handler_(event_handler) {}
FakeBluetoothChooser::~FakeBluetoothChooser() = default;

void FakeBluetoothChooser::WaitForEvents(uint32_t num_of_events,
                                         WaitForEventsCallback callback) {
  NOTREACHED();
}

void FakeBluetoothChooser::SelectPeripheral(
    const std::string& peripheral_address,
    SelectPeripheralCallback callback) {
  NOTREACHED();
}

void FakeBluetoothChooser::Cancel(CancelCallback callback) {
  NOTREACHED();
}

void FakeBluetoothChooser::Rescan(RescanCallback callback) {
  NOTREACHED();
}

}  // namespace content
