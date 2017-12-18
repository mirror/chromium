// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/public/cpp/test_support/test_environment.h"

#include "base/logging.h"
#include "mojo/edk/embedder/embedder.h"

namespace mojo {
namespace test {

TestEnvironment::TestEnvironment() : mojo_ipc_thread_("MojoIpcThread") {}

TestEnvironment::~TestEnvironment() = default;

void TestEnvironment::SetUp() {
  mojo::edk::Init();
  mojo_ipc_thread_.StartWithOptions(
      base::Thread::Options(base::MessageLoop::TYPE_IO, 0));
  mojo_ipc_support_.reset(new mojo::edk::ScopedIPCSupport(
      mojo_ipc_thread_.task_runner(),
      mojo::edk::ScopedIPCSupport::ShutdownPolicy::FAST));
  VLOG(1) << "Mojo initialized";
}

void TestEnvironment::TearDown() {
  mojo_ipc_support_.reset();
  VLOG(1) << "Mojo IPC tear down";
}

}  // namespace test
}  // namespace mojo
