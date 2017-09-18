// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/exo/wayland/clients/test/wayland_client_test.h"

#include "base/command_line.h"
#include "base/threading/thread.h"
#include "components/exo/display.h"
#include "components/exo/file_helper.h"
#include "components/exo/wayland/server.h"
#include "components/exo/wm_helper_ash.h"
#include "ui/wm/core/wm_core_switches.h"

namespace exo {

class WaylandClientTest::WaylandWatcher
    : public base::MessagePumpLibevent::Watcher {
 public:
  explicit WaylandWatcher(exo::wayland::Server* server)
      : controller_(FROM_HERE), server_(server) {
    base::MessageLoopForUI::current()->WatchFileDescriptor(
        server_->GetFileDescriptor(),
        true,  // persistent
        base::MessagePumpLibevent::WATCH_READ, &controller_, this);
  }

  // base::MessagePumpLibevent::Watcher:
  void OnFileCanReadWithoutBlocking(int fd) override {
    server_->Dispatch(base::TimeDelta());
    server_->Flush();
  }
  void OnFileCanWriteWithoutBlocking(int fd) override { NOTREACHED(); }

 private:
  base::MessagePumpLibevent::FileDescriptorWatcher controller_;
  exo::wayland::Server* const server_;

  DISALLOW_COPY_AND_ASSIGN(WaylandWatcher);
};

WaylandClientTest::WaylandClientTest() {}

WaylandClientTest::~WaylandClientTest() {}

void WaylandClientTest::RunTest(const TestEntry& entry) {
  client_thread_->message_loop()->task_runner()->PostTask(
      FROM_HERE, base::Bind(&WaylandClientTest::RunTestOnClientThread,
                            base::Unretained(this), entry));
  RunMessageLoop();
}

void WaylandClientTest::SetUp() {
  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
  // Disable window animation when running tests.
  command_line->AppendSwitch(wm::switches::kWindowAnimationsDisabled);

  AshTestBase::SetUp();
  UpdateDisplay("800x600");

  wm_helper_ = base::MakeUnique<WMHelperAsh>();
  WMHelper::SetInstance(wm_helper_.get());
  display_ = base::MakeUnique<Display>(nullptr, nullptr);
  wayland_server_ = exo::wayland::Server::Create(display_.get());
  DCHECK(wayland_server_);
  wayland_watcher_ = base::MakeUnique<WaylandWatcher>(wayland_server_.get());

  client_thread_ = base::MakeUnique<base::Thread>("WaylandClient");
  client_thread_->Start();
}

void WaylandClientTest::TearDown() {
  client_thread_->FlushForTesting();
  client_thread_.reset();
  wayland_watcher_.reset();
  wayland_server_.reset();
  display_.reset();
  WMHelper::SetInstance(nullptr);
  wm_helper_.reset();
  AshTestBase::TearDown();
}

void WaylandClientTest::RunMessageLoop() {
  DCHECK(!run_loop_);
  run_loop_ = base::MakeUnique<base::RunLoop>();
  run_loop_->Run();
  run_loop_.reset();
}

void WaylandClientTest::QuitMessageLoop() {
  DCHECK(run_loop_);
  run_loop_->Quit();
}

void WaylandClientTest::RunTestOnClientThread(const TestEntry& entry) {
  result_ = entry.Run();
  QuitMessageLoop();
}

}  // namespace exo
