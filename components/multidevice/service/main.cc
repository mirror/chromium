// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/service_manager/public/c/main.h"
#include "components/multidevice/service/multidevice_service.h"
#include "services/service_manager/public/cpp/service_runner.h"

class MultiDeviceTestService : public multidevice::MultiDeviceService {
 public:
  explicit MultiDeviceTestService() {}
  ~MultiDeviceTestService() override {}
};

MojoResult ServiceMain(MojoHandle service_request_handle) {
  service_manager::ServiceRunner runner(new MultiDeviceTestService());
  return runner.Run(service_request_handle);
}
