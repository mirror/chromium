// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_EXO_DATA_EVEMT_DISPATCHER_H_
#define COMPONENTS_EXO_DATA_EVEMT_DISPATCHER_H_

#include "base/containers/flat_set.h"
#include "base/macros.h"

namespace exo {

class DataDevice;

// Object dispatching events to data_device.
class DataEventDispatcher {
 public:
  DataEventDispatcher();
  ~DataEventDispatcher();

  // Adds |data_device| to the manager.
  void AddDataDevice(DataDevice* data_device);

  // Removes |data_device| from the manager.
  void RemoveDataDevice(DataDevice* data_device);

 private:
  base::flat_set<DataDevice*> data_devices_;

  DISALLOW_COPY_AND_ASSIGN(DataEventDispatcher);
};

}  // namespace exo

#endif  // COMPONENTS_EXO_DATA_EVEMT_DISPATCHER_H_
