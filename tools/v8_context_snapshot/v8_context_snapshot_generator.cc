// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/at_exit.h"
#include "base/command_line.h"
#include "base/files/file_util.h"
#include "base/message_loop/message_loop.h"
#include "base/task_scheduler/task_scheduler.h"
#include "base/threading/thread_task_runner_handle.h"
#include "gin/v8_initializer.h"
#include "mojo/edk/embedder/embedder.h"
#include "services/service_manager/public/cpp/binder_registry.h"
#include "third_party/WebKit/Source/platform/WebTaskRunner.h"
#include "third_party/WebKit/public/platform/WebThread.h"
#include "third_party/WebKit/public/web/WebKit.h"
#include "third_party/WebKit/public/web/WebV8ContextSnapshot.h"
#include "v8/include/v8.h"

namespace {

class SnapshotWebTaskRunner : public blink::WebTaskRunner {
 public:
  SnapshotWebTaskRunner() : task_runner_(base::ThreadTaskRunnerHandle::Get()) {}

  double MonotonicallyIncreasingVirtualTimeSeconds() const override {
    NOTIMPLEMENTED();
    return 0;
  }

  bool RunsTasksInCurrentSequence() const override {
    return task_runner_->RunsTasksInCurrentSequence();
  }

  bool PostNonNestableDelayedTask(const base::Location& location,
                                  base::OnceClosure task,
                                  base::TimeDelta delay) override {
    return task_runner_->PostNonNestableDelayedTask(location, std::move(task),
                                                    delay);
  }

  bool PostDelayedTask(const base::Location& location,
                       base::OnceClosure task,
                       base::TimeDelta delay) override {
    return task_runner_->PostDelayedTask(location, std::move(task), delay);
  }

 private:
  ~SnapshotWebTaskRunner() override {}
  scoped_refptr<base::SingleThreadTaskRunner> task_runner_;
};

class SnapshotThread : public blink::WebThread {
 public:
  SnapshotThread() : task_runner_(new SnapshotWebTaskRunner()) {}
  ~SnapshotThread() override {}
  bool IsCurrentThread() const override { return true; }
  blink::WebScheduler* Scheduler() const override { return nullptr; }
  blink::WebTaskRunner* GetWebTaskRunner() override {
    return task_runner_.get();
  }

 private:
  scoped_refptr<SnapshotWebTaskRunner> task_runner_;
};

class SnapshotPlatform final : public blink::Platform {
 public:
  bool IsTakingV8ContextSnapshot() override { return true; }
  blink::WebThread* CurrentThread() override {
    static SnapshotThread dummy_thread;
    return &dummy_thread;
  }
};

}  // namespace

// This program takes a snapshot of V8 contexts and writes it out as a file.
// The snapshot file is consumed by Blink.
//
// Usage:
// % v8_context_snapshot_generator --output_file=<filename>
int main(int argc, char** argv) {
  base::AtExitManager at_exit;
  base::CommandLine::Init(argc, argv);
#ifdef V8_USE_EXTERNAL_STARTUP_DATA
  gin::V8Initializer::LoadV8Snapshot();
  gin::V8Initializer::LoadV8Natives();
#endif

  // Set up environment to make Blink and V8 workable.
  base::MessageLoop message_loop;
  base::TaskScheduler::CreateAndStartWithDefaultParams("TakeSnapshot");
  mojo::edk::Init();

  // Take a snapshot.
  SnapshotPlatform platform;
  service_manager::BinderRegistry empty_registry;
  blink::Initialize(&platform, &empty_registry);
  v8::StartupData blob = blink::WebV8ContextSnapshot::TakeSnapshot();

  // Save the snapshot as a file. Filename is given in a command line option.
  base::FilePath file_path =
      base::CommandLine::ForCurrentProcess()->GetSwitchValuePath("output_file");
  CHECK(!file_path.empty());
  CHECK_LT(0, base::WriteFile(file_path, blob.data, blob.raw_size));

  delete[] blob.data;

  // v8::SnapshotCreator used in WebV8ContextSnapshot makes it complex how to
  // manage lifetime of v8::Isolate, gin::IsolateHolder, and
  // blink::V8PerIsolateData. Now we complete all works at this point, and can
  // exit without releasing all those instances correctly.
  _exit(0);
}
