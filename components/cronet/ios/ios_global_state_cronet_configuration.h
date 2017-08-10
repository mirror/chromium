// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_CRONET_IOS_IOS_GLOBAL_STATE_CRONET_CONFIGURATION_H_
#define COMPONENTS_CRONET_IOS_IOS_GLOBAL_STATE_CRONET_CONFIGURATION_H_

#include "base/threading/thread.h"

namespace ios_global_state {

// A configuration object which provides a task runner for the IO thread from
// |network_io_thread_|.
class IOSGlobalStateCronetConfiguration {
 public:
  IOSGlobalStateCronetConfiguration();
  ~IOSGlobalStateCronetConfiguration();

  // Returns a TaskRunner for running tasks on the IO thread.
  scoped_refptr<base::SingleThreadTaskRunner> GetIOThreadTaskRunner();

 private:
  // |network_io_thread_| provides a |task_runner| for running tasks associated
  // with the IO thread.
  std::unique_ptr<base::Thread> network_io_thread_;
};

}  // namespace ios_global_state

#endif  // COMPONENTS_CRONET_IOS_IOS_GLOBAL_STATE_CRONET_CONFIGURATION_H_
