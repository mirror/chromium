// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TOOLS_CYGPROFILE_LIGHTWEIGHT_CYGPROFILE_H_
#define TOOLS_CYGPROFILE_LIGHTWEIGHT_CYGPROFILE_H_

#include "build/build_config.h"

#if !defined(OS_ANDROID)
#error This is only supported on Android.
#endif

namespace cygprofile {
// Starts profiling the symbols, and dumps the results after a given duration.
void LogAndDump(int duration_seconds);
}  // namespace cygprofile

#endif  // TOOLS_CYGPROFILE_LIGHTWEIGHT_CYGPROFILE_H_
