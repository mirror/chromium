// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/dbus_thread/dbus_thread.h"

namespace dbus_thread {

namespace {

base::LazySingleThreadTaskRunner g_dbus_thread_task_runner =
    LAZY_SINGLE_THREAD_TASK_RUNNER_INITIALIZER({base::MayBlock()});

}  // namespace

scoped_refptr<base::SingleThreadTaskRunner> GetTaskRunner() {
  return g_dbus_thread_task_runner.Get();
}

}  // namespace dbus_thread
