// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/u2f/mock_u2f_discovery.h"

#include <utility>

#include "base/threading/thread_task_runner_handle.h"

namespace device {

MockU2fDiscovery::MockU2fDiscovery() = default;

MockU2fDiscovery::~MockU2fDiscovery() = default;

void MockU2fDiscovery::DiscoverDevice(std::unique_ptr<U2fDevice> device,
                                      DeviceStatus status) {
  callback_.Run(std::move(device), status);
}

// static
void MockU2fDiscovery::StartSuccess(StartedCallback started) {
  std::move(started).Run(true);
}

// static
void MockU2fDiscovery::StartFailure(StartedCallback started) {
  std::move(started).Run(false);
}

// static
void MockU2fDiscovery::StartSuccessAsync(StartedCallback started) {
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE,
      base::BindOnce(&MockU2fDiscovery::StartSuccess, std::move(started)));
}

// static
void MockU2fDiscovery::StartFailureAsync(StartedCallback started) {
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE,
      base::BindOnce(&MockU2fDiscovery::StartFailure, std::move(started)));
}

}  // namespace device
