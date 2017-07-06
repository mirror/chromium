// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_CRONET_INIT_TASK_SCHEDULER_H_
#define COMPONENTS_CRONET_INIT_TASK_SCHEDULER_H_

namespace cronet {

// Initializes the TaskScheduler singleton in a manner appropriate for cronet.
// This should be called once, early in startup. A corresponding call to
// ShutdownTaskScheduler must be called during shutdown.
void InitTaskScheduler();

void ShutdownTaskScheduler();

}  // namespace cronet

#endif  // COMPONENTS_CRONET_INIT_TASK_SCHEDULER_H_
