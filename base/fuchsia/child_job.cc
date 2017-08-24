// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/fuchsia/child_job.h"

#include <magenta/process.h>
#include <magenta/syscalls.h>

#include "base/logging.h"

namespace base {

namespace {
static mx_handle_t g_job = mx_job_default();
}  // namespace

mx_handle_t GetDefaultJob() {
  return g_job;
}

void SetDefaultJob(mx_handle_t job) {
  g_job = job;
}

}  // namespace base
