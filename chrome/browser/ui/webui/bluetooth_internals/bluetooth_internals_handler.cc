// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/bluetooth_internals/bluetooth_internals_handler.h"

#include "base/macros.h"
#include "base/strings/string16.h"
#include "base/strings/utf_string_conversions.h"
#include "url/gurl.h"

BluetoothInternalsHandler::BluetoothInternalsHandler(
    mojom::BluetoothInternalsHandlerRequest request)
    : binding_(this, std::move(request)) {}

BluetoothInternalsHandler::~BluetoothInternalsHandler() {}

void BluetoothInternalsHandler::GetBluetoothAdapter(
    GetBluetoothAdapterCallback callback) {
  std::move(callback).Run();
}
