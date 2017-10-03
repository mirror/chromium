// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/exo/wayland/clients/rects.h"

#include <limits>

#include "base/at_exit.h"
#include "base/command_line.h"
#include "base/logging.h"
#include "base/strings/string_number_conversions.h"

namespace switches {

// Specifies the maximum number of pending frames.
const char kMaxFramesPending[] = "max-frames-pending";

// Specifies the number of rotating rects to draw.
const char kNumRects[] = "num-rects";

// Enables benchmark mode and specifies the number of benchmark runs to
// perform before client will exit. Client will print the results to
// standard output as a tab seperated list.
//
//  The output format is:
//   "frames wall-time-ms cpu-time-ms"
const char kBenchmark[] = "benchmark";

// Specifies the number of milliseconds to use as benchmark interval.
const char kBenchmarkInterval[] = "benchmark-interval";

// Specifies if FPS counter should be shown.
const char kShowFpsCounter[] = "show-fps-counter";

}  // namespace switches

int main(int argc, char* argv[]) {
  base::AtExitManager exit_manager;
  base::CommandLine::Init(argc, argv);
  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();

  exo::wayland::clients::ClientBase::InitParams params;
  if (!params.FromCommandLine(*command_line))
    return 1;

  size_t max_frames_pending = 0;
  if (command_line->HasSwitch(switches::kMaxFramesPending) &&
      (!base::StringToSizeT(
          command_line->GetSwitchValueASCII(switches::kMaxFramesPending),
          &max_frames_pending))) {
    LOG(ERROR) << "Invalid value for " << switches::kMaxFramesPending;
    return 1;
  }

  size_t num_rects = 1;
  if (command_line->HasSwitch(switches::kNumRects) &&
      !base::StringToSizeT(
          command_line->GetSwitchValueASCII(switches::kNumRects), &num_rects)) {
    LOG(ERROR) << "Invalid value for " << switches::kNumRects;
    return 1;
  }

  size_t num_benchmark_runs = 0;
  if (command_line->HasSwitch(switches::kBenchmark) &&
      (!base::StringToSizeT(
          command_line->GetSwitchValueASCII(switches::kBenchmark),
          &num_benchmark_runs))) {
    LOG(ERROR) << "Invalid value for " << switches::kBenchmark;
    return 1;
  }

  size_t benchmark_interval_ms = 5000;  // 5 seconds.
  if (command_line->HasSwitch(switches::kBenchmarkInterval) &&
      (!base::StringToSizeT(
          command_line->GetSwitchValueASCII(switches::kBenchmarkInterval),
          &benchmark_interval_ms))) {
    LOG(ERROR) << "Invalid value for " << switches::kBenchmarkInterval;
    return 1;
  }

  exo::wayland::clients::Rects client;
  if (!client.Init(params))
    return 1;

  bool result =
      client.Run(std::numeric_limits<size_t>::max(), max_frames_pending,
                 num_rects, num_benchmark_runs,
                 base::TimeDelta::FromMilliseconds(benchmark_interval_ms),
                 command_line->HasSwitch(switches::kShowFpsCounter));
  return result ? 0 : 1;
}
