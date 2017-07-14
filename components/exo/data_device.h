// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_EXO_DATA_DEVICE_H_
#define COMPONENTS_EXO_DATA_DEVICE_H_

#include <cstdint>

#include "base/macros.h"

namespace exo {

class DataDeviceDelegate;
class DataSource;
class Surface;

// Object representing an entry point for data transfer.
// Requests and events are compatible wl_data_device. See wl_data_device for
// details.
class DataDevice {
 public:
  explicit DataDevice(DataDeviceDelegate* delegate);
  ~DataDevice();

  // Starts drag-and-drop operation
  void StartDrag(const DataSource* source,
                 Surface* origin,
                 Surface* icon,
                 uint32_t serial);

  // Copies data to the selection
  void SetSelection(const DataSource& source, uint32_t serial);

 private:
  DataDeviceDelegate* const delegate_;

  DISALLOW_COPY_AND_ASSIGN(DataDevice);
};

}  // namespace exo

#endif  // COMPONENTS_EXO_DATA_DEVICE_H_
