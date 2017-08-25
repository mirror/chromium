// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_FUCHSIA_CHILD_JOB_H_
#define BASE_FUCHSIA_CHILD_JOB_H_

#include <magenta/types.h>

#include "base/fuchsia/scoped_mx_handle.h"

namespace base {

// Returns the handle to the Fuchsia job used for creating new child processes,
// and looking them up by their process IDs.
mx_handle_t GetDefaultJob();

// Sets the default handle to use when launching processes and looking them up
// by their process IDs.
void SetDefaultJob(ScopedMxHandle job);

}  // namespace base

#endif  // BASE_FUCHSIA_CHILD_JOB_H_
