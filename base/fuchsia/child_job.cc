// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/fuchsia/child_job.h"

#include <magenta/process.h>
#include <magenta/syscalls.h>

#include "base/logging.h"

namespace base {
namespace {
static mx_handle_t g_job_handle = 0;
}  // namespace

mx_handle_t GetChildProcessJob() {
  if (!g_job_handle) {
    mx_status_t result = mx_job_create(mx_job_default(), 0, &g_job_handle);
    CHECK_EQ(MX_OK, result)
        << "Error when creating a new child job, result: " << result;
  }
  return g_job_handle;
}

}  // namespace base
