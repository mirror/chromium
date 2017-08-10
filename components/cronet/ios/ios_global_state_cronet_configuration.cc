// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/cronet/ios/ios_global_state_cronet_configuration.h"

namespace ios_global_state {

IOSGlobalStateCronetConfiguration::IOSGlobalStateCronetConfiguration()
    : network_io_thread_(new base::Thread("Chrome Network IO Thread")) {
  network_io_thread_->StartWithOptions(
      base::Thread::Options(base::MessageLoop::TYPE_IO, 0));
}

IOSGlobalStateCronetConfiguration::~IOSGlobalStateCronetConfiguration() =
    default;

scoped_refptr<base::SingleThreadTaskRunner>
IOSGlobalStateCronetConfiguration::GetIOThreadTaskRunner() {
  return network_io_thread_->task_runner();
}

}  // namespace ios_global_state
