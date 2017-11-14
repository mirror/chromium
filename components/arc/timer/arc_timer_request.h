// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_ARC_TIMER_ARC_TIMER_REQUEST_H_
#define COMPONENTS_ARC_TIMER_ARC_TIMER_REQUEST_H_

#include <stdint.h>

#include <base/files/file.h>

// Typemapping for |ArcTimerRequest| in timer.mojom
namespace arc {

struct ArcTimerRequest {
  int32_t clock_id;
  base::ScopedFD expiration_fd;
};

}  // namespace arc

#endif  // COMPONENTS_ARC_TIMER_ARC_TIMER_REQUEST_H_
