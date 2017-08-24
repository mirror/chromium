// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <magenta/syscalls.h>

namespace base {

// Returns a handle to the Fuchsia job used to create new child processes.
mx_handle_t GetDefaultJob();

// Overrides the value for GetDefaultJob(). The browser process uses this hook
// to register a new job which contains all child and descendent processes.
void SetDefaultJob(mx_handle_t job);

}  // namespace base
