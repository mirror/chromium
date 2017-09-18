// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/bind.h"
#include "base/command_line.h"
#include "components/exo/wayland/clients/simple.h"
#include "components/exo/wayland/clients/test/wayland_client_test.h"
#include "testing/perf/perf_test.h"

namespace {

const int kWarmUpFrames = 20;
const int kTestFrames = 600;

bool SimpleTest() {
  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
  exo::wayland::clients::ClientBase::InitParams params;
  if (!params.FromCommandLine(*command_line))
    return false;

  exo::wayland::clients::Simple client;
  if (!client.Init(params))
    return false;

  client.Run(kWarmUpFrames);

  auto start_time = base::Time::Now();
  client.Run(kTestFrames);
  auto time_delta = base::Time::Now() - start_time;
  float fps = kTestFrames / time_delta.InSecondsF();
  perf_test::PrintResult("WaylandClientPerfTests", "", "Simple", fps,
                         "frames/s", true);
  return true;
}

using WaylandClientPerfTests = exo::WaylandClientTest;

TEST_F(WaylandClientPerfTests, Simple) {
  RunTest(base::Bind(SimpleTest));
  EXPECT_TRUE(result());
}

}  // namespace
