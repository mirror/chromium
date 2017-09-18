// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_U2F_U2F_BLE_UUIDS_H_
#define DEVICE_U2F_U2F_BLE_UUIDS_H_

namespace device {

static constexpr const char* U2F_SERVICE_UUID =
    "0000fffd-0000-1000-8000-00805f9b34fb";
static constexpr const char* U2F_CONTROL_POINT_UUID =
    "f1d0fff1-deaa-ecee-b42f-c9ba7ed623bb";
static constexpr const char* U2F_STATUS_UUID =
    "f1d0fff2-deaa-ecee-b42f-c9ba7ed623bb";
static constexpr const char* U2F_CONTROL_POINT_LENGTH_UUID =
    "f1d0fff3-deaa-ecee-b42f-c9ba7ed623bb";

}  // namespace device

#endif  // DEVICE_U2F_U2F_BLE_UUIDS_H_
