// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/viz/test/fake_delay_based_time_source.h"

namespace viz {

void FakeDelayBasedTimeSourceClient::OnTimerTick() {
  tick_called_ = true;
}

base::TimeTicks FakeDelayBasedTimeSource::Now() const {
  return now_;
}

}  // namespace viz
