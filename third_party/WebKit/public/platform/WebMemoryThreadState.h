// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WebMemoryThreadState_h
#define WebMemoryThreadState_h

#include "public/platform/WebCommon.h"

namespace blink {

class WebMemoryThreadState {
 public:
  // Sets the threshold to force memory pressure GC in blink. Default threshold
  // is set to 300MB.
  BLINK_PLATFORM_EXPORT static void SetForceMemoryPressureThreshold(
      size_t threshold);

  // Returns the current threshold of the memory pressure GC.
  BLINK_PLATFORM_EXPORT static size_t GetForceMemoryPressureThreshold();
};

}  // namespace blink

#endif
