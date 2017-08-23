// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <magenta/syscalls.h>

namespace base {

// Returns a handle to the Fuchsia job used to create new child processes.
mx_handle_t GetChildProcessJob();

}  // namespace base
