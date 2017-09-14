// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENT_EXO_WAYLAND_CLIENTS_TEST_WAYLAND_CLIENT_TEST_H_
#define COMPONENT_EXO_WAYLAND_CLIENTS_TEST_WAYLAND_CLIENT_TEST_H_

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

class WaylandClientTest : public ash::AshTestBase {
 public:
  WaylandClientTest();
  ~WaylandClientTest() override;

  using TestEntry = base::Callback<bool(void)>;
  void RunTest(const TestEntry& entry);

  bool result() const { return result_; }

 private:
  class WaylandWatcher;

  void RunMessageLoop();
  void QuitMessageLoop();

  // Overridden from AshTestBase:
  void SetUp() override;
  void TearDown() override;

  void RunTestOnClientThread(const TestEntry& entry);

  std::unique_ptr<WMHelper> wm_helper_;
  std::unique_ptr<Display> display_;
  std::unique_ptr<wayland::Server> wayland_server_;
  std::unique_ptr<WaylandWatcher> wayland_watcher_;
  std::unique_ptr<base::RunLoop> run_loop_;
  std::unique_ptr<base::Thread> client_thread_;
  bool result_ = false;

  DISALLOW_COPY_AND_ASSIGN(WaylandClientTest);
};

}  // namespace exo

#endif  // COMPONENT_EXO_WAYLAND_CLIENTS_TEST_WAYLAND_CLIENT_TEST_H_
