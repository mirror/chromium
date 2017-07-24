// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_COMMON_PROFILING_PROFILING_CONSTANTS_H_
#define CHROME_COMMON_PROFILING_PROFILING_CONSTANTS_H_

#include "build/build_config.h"

#if defined(OS_POSIX)
#if defined(OS_ANDROID)
#include "chrome/common/descriptors_android.h"
#else
#include "content/public/common/content_descriptors.h"
#endif
#endif

namespace profiling {

// Name of the profiling control Mojo service.
extern const char kProfilingControlPipeName[];

#if defined(OS_POSIX)
// TODO(ajwong): Hack! This should be located in something
// global to the chrome module.
enum {
#if defined(OS_ANDROID)
  kProfilingDataPipe = kAndroidMaxDescriptor + 1,
#else
  kProfilingDataPipe = kContentIPCDescriptorMax + 1,
#endif
};
#endif

}  // namespace profiling

#endif  // CHROME_COMMON_PROFILING_PROFILING_CONSTANTS_H_
