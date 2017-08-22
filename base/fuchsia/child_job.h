// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <magenta/syscalls.h>

namespace base {

// Returns a handle to the Fuchsia job used to create new child processes.
mx_handle_t GetChildProcessJob();

// Specifies that a new job should be used when launching new child processes,
// instead of the parent process' default job.
// GetChildProcessJob() will return a handle to the new job afterward.
// Should be called at most once on startup.
void InitNewChildProcessJob();

}  // namespace base
