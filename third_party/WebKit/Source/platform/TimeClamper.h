// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TimeClamper_h
#define TimeClamper_h

#include "base/macros.h"
#include "platform/PlatformExport.h"

#include <stdint.h>

namespace blink {

class PLATFORM_EXPORT TimeClamper {
 public:
  static constexpr double kResolutionSeconds = 0.0001;

  TimeClamper();
  double ClampTimeResolution(double time_seconds) const;

 private:
  inline double ThresholdFor(double clamped_time) const;
  static inline double ToDouble(uint64_t value);
  static inline uint64_t MurmurHash3(uint64_t value);

  uint64_t secret_;

  DISALLOW_COPY_AND_ASSIGN(TimeClamper);
};

}  // namespace blink

#endif  // TimeClamper_h
