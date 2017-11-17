// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_GAMEPAD_GAMEPAD_UMA_H_
#define DEVICE_GAMEPAD_GAMEPAD_UMA_H_

#include "base/strings/string_piece.h"

namespace device {

// Records the vendor/product IDs for a connected gamepad and whether a mapping
// function was used for this gamepad.
void RecordConnectedGamepadInfo(const base::StringPiece& vendor_id,
                                const base::StringPiece& product_id,
                                bool has_standard_mapping);

}  // namespace device

#endif  // DEVICE_GAMEPAD_GAMEPAD_UMA_H_
