// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/single_thread_task_runner.h"

namespace base {

scoped_refptr<SingleThreadTaskRunner> SingleThreadTaskRunner::Clone(
    const tracked_objects::Location& from_here) {
  return this;
}

}  // namespace base
