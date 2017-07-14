// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_EXO_DATA_DEVICE_DELEGATE_H_
#define COMPONENTS_EXO_DATA_DEVICE_DELEGATE_H_

#include <string>
#include <vector>

namespace exo {

class DataDevice;
class DataOffer;

// Provides protocol specific implementations of DataDevice.
class DataDeviceDelegate {
 public:
  // Notifies when DataDevice is being destroyed.
  virtual void OnDataDeviceDestroying(DataDevice*) = 0;

  // Notifies DataOffer object having |mime_types|, |source_actions| and
  // |dnd_action|.
  virtual DataOffer* OnDataOffer(std::vector<std::string> mime_types,
                                 uint32_t source_actions,
                                 uint32_t dnd_action) = 0;

  // Notifies the pointer enters to the surface.
  virtual void OnEnter(uint32_t serial,
                       Surface* surface,
                       float x,
                       float y,
                       const class DataOffer& data_offer) = 0;

  // Notifies the pointer leaves.
  virtual void OnLeave() = 0;

  // Notifies the pointer moves.
  virtual void OnMotion(uint32_t time, float x, float y) = 0;

  // Notifies the drop operation was performed.
  virtual void OnDrop() = 0;

  // Notifies the data is pasted on the DataDevice.
  virtual void OnSelection(const class DataOffer& data_offer) = 0;

 protected:
  virtual ~DataDeviceDelegate() = default;
};

}  // namespace exo

#endif  // COMPONENTS_EXO_DATA_DEVICE_DELEGATE_H_
