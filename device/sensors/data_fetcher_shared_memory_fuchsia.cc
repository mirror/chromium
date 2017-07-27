// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/sensors/data_fetcher_shared_memory.h"

#include "base/logging.h"

namespace device {

DataFetcherSharedMemory::DataFetcherSharedMemory() {}

DataFetcherSharedMemory::~DataFetcherSharedMemory() {}

bool DataFetcherSharedMemory::Start(ConsumerType consumer_type, void* buffer) {
  NOTIMPLEMENTED();
  return false;
}

bool DataFetcherSharedMemory::Stop(ConsumerType consumer_type) {
  NOTIMPLEMENTED();
  return false;
}

}  // namespace device
