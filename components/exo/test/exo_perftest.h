// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENT_EXO_TEST_EXO_PERF_TEST_H_
#define COMPONENT_EXO_TEST_EXO_PERF_TEST_H_

#include "ash/test/ash_test_base.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace base {
class RunLoop;
class Thread;
}  // namespace base

namespace exo {
namespace wayland {
class Server;
}

class Display;
class WMHelper;

class ExoPerfTest : public ash::AshTestBase {
 public:
  ExoPerfTest();
  ~ExoPerfTest() override;

  void RunMessageLoop();
  void QuitMessageLoop();
  using ClientEntry = base::Callback<int(void)>;
  void RunClient(const ClientEntry& client_entry);

  int result() const { return result_; }

  // AshTestBase:
  void SetUp() override;
  void TearDown() override;

 private:
  class WaylandWatcher;

  void RunClientOnClientThread(const ClientEntry& client_entry);

  std::unique_ptr<WMHelper> wm_helper_;
  std::unique_ptr<Display> display_;
  std::unique_ptr<wayland::Server> wayland_server_;
  std::unique_ptr<WaylandWatcher> wayland_watcher_;
  std::unique_ptr<base::RunLoop> run_loop_;
  std::unique_ptr<base::Thread> client_thread_;
  int result_ = 1;

  DISALLOW_COPY_AND_ASSIGN(ExoPerfTest);
};

}  // namespace exo

#endif  // COMPONENT_EXO_TEST_EXO_PERF_TEST_H_
