// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/fuchsia/child_job.h"

#include <magenta/process.h>
#include <magenta/syscalls.h>

#include "base/logging.h"

namespace base {

namespace {
static mx_handle_t g_job = MX_HANDLE_INVALID;
}  // namespace

mx_handle_t GetDefaultJob() {
  if (g_job == MX_HANDLE_INVALID) {
    g_job = mx_job_default();
  }
  return g_job;
}

void SetDefaultJob(ScopedMxHandle job) {
  g_job = job.release();
}

}  // namespace base
