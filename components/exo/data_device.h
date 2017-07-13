// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_EXO_DATA_DEVICE_H_
#define COMPONENTS_EXO_DATA_DEVICE_H_

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"

namespace exo {

class DataOffer;
class DataSource;
class Surface;

// Provides protocol specific implementations of DataDevice.
class DataDeviceDelegate {
 public:
  virtual ~DataDeviceDelegate();
  virtual const DataOffer& DataOffer(const DataSource& source) = 0;
  virtual void Enter(uint32_t serial,
                     Surface* surface,
                     float x,
                     float y,
                     const class DataOffer& data_offer) = 0;
  virtual void Leave() = 0;
  virtual void Motion(uint32_t time, float x, float y) = 0;
  virtual void Drop() = 0;
  virtual void Selection(const class DataOffer& data_offer) = 0;
};

// Object representing the entry point for data transfer.
// Requests and events are compatible wl_data_device. See wl_data_device for
// details.
class DataDevice {
 public:
  explicit DataDevice(std::unique_ptr<DataDeviceDelegate> delegate);

  void StartDrag(const DataSource* source,
                 Surface* origin,
                 Surface* icon,
                 uint32_t serial);
  void SetSelection(const DataSource& source, uint32_t serial);

 private:
  std::unique_ptr<DataDeviceDelegate> delegate_;

  DISALLOW_COPY_AND_ASSIGN(DataDevice);
};

}  // namespace exo

#endif  // COMPONENTS_EXO_DATA_DEVICE_H_
