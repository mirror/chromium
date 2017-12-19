// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/views/controls/label.h"

#include "base/strings/utf_string_conversions.h"
#include "cc/base/lap_timer.h"
#include "testing/perf/perf_test.h"
#include "ui/views/test/views_test_base.h"

namespace views {

using LabelPerfTest = ViewsTestBase;

TEST_F(LabelPerfTest, GetPreferredSize) {
  Label label;

  // Alternate between two strings to ensure caches are cleared.
  base::string16 string1 = base::ASCIIToUTF16("boring ascii string");
  base::string16 string2 = base::ASCIIToUTF16("another uninteresting sequence");

  constexpr int kLaps = 10000;  // Aim to run in under 5 seconds.
  constexpr int kWarmupRuns = 5;

  // The time limit is unused. Use kLaps for the check interval so the time is
  // only measured once.
  cc::LapTimer timer(kWarmupRuns, base::TimeDelta(), kLaps);
  for (int i = 0; i < kLaps + kWarmupRuns; ++i) {
    label.SetText(i % 2 == 0 ? string1 : string2);
    label.GetPreferredSize();
    timer.NextLap();
  }
  perf_test::PrintResult("LabelPerfTest", std::string(), "GetPreferredSize",
                         timer.LapsPerSecond(), "runs/s", true);
}

}  // namespace views
