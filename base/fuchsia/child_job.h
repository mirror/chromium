// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <magenta/syscalls.h>

#include "base/lazy_instance.h"

namespace base {

mx_handle_t GetChildProcessJob();

void InitNewChildProcessJob();

}  // namespace base
