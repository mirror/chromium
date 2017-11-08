// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_ARC_COMMON_VIDEO_ACCELERATOR_H_
#define COMPONENTS_ARC_COMMON_VIDEO_ACCELERATOR_H_

#include <stdint.h>

namespace arc {

struct VideoFramePlane {
  int32_t offset;  // in bytes
  int32_t stride;  // in bytes
};

}  // namespace arc

#endif  // COMPONENTS_ARC_COMMON_VIDEO_ACCELERATOR_H_
