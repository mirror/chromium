// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/host/process_metrics.h"

#include "base/base_paths.h"
#include "base/command_line.h"
#include "base/path_service.h"
#include "base/process/launch.h"
#include "base/process/process.h"
#include "remoting/host/current_process_stats_agent.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace remoting {

TEST(ProcessMetricsTest, GetMetricsOfANewProcess) {
  base::FilePath directory;
  ASSERT_TRUE(base::PathService::Get(base::DIR_EXE, &directory));
#if defined(OS_WIN)
  base::CommandLine command(
      directory.Append(FILE_PATH_LITERAL("process_metrics_test_stub.exe")));
#else
  base::CommandLine command(
      directory.Append(FILE_PATH_LITERAL("process_metrics_test_stub")));
#endif
  ProcessMetrics metrics("test_process",
                         base::LaunchProcess(command, base::LaunchOptions()));

  ASSERT_TRUE(metrics.IsValid());
  ASSERT_TRUE(metrics.IsRunning());
  protocol::ProcessResourceUsage usage = metrics.GetResourceUsage();
  ASSERT_EQ("test_process", usage.process_name());
  ASSERT_TRUE(usage.has_processor_usage());
  ASSERT_TRUE(usage.has_working_set_size());
  ASSERT_TRUE(usage.has_pagefile_size());

  ASSERT_TRUE(metrics.TerminateForTesting());
  ASSERT_FALSE(metrics.IsRunning());
  usage = metrics.GetResourceUsage();
  ASSERT_FALSE(usage.has_process_name());
}

TEST(ProcessMetricsTest, GetCurrentProcessMetrics) {
  CurrentProcessStatsAgent agent("test_process");
  protocol::ProcessResourceUsage usage = agent.GetResourceUsage();
  ASSERT_EQ("test_process", usage.process_name());
  ASSERT_TRUE(usage.has_processor_usage());
  ASSERT_TRUE(usage.has_working_set_size());
  ASSERT_TRUE(usage.has_pagefile_size());
}

}  // namespace remoting
