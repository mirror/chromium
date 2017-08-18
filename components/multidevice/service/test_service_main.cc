// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "base/logging.h"
#include "components/multidevice/service/multidevice_service.h"
#include "services/service_manager/public/c/main.h"
#include "services/service_manager/public/cpp/service_runner.h"

// class MultiDeviceTestService : public multidevice::MultiDeviceService {
//  public:
//   explicit MultiDeviceTestService() {}
//   ~MultiDeviceTestService() override {}
// };

MojoResult ServiceMain(MojoHandle service_request_handle) {
  LOG(ERROR) << "hello!!";
  service_manager::ServiceRunner runner(new multidevice::MultiDeviceService());
  return runner.Run(service_request_handle);
}
