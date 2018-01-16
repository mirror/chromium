// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/command_line.h"
#include "base/run_loop.h"
#include "base/synchronization/waitable_event.h"
#include "content/browser/browser_main_loop.h"
#include "content/browser/gpu/gpu_main_thread_factory.h"
#include "content/browser/gpu/gpu_process_host.h"
#include "content/gpu/in_process_gpu_thread.h"
#include "content/public/common/content_switches.h"
#include "content/public/test/content_browser_test.h"

namespace {

static bool slow_gpu_thread_created = false;

using base::WaitableEvent;
using content::InProcessGpuThread;
using content::InProcessChildThreadParams;
using content::GpuProcessHost;
class SlowGpuThread : public InProcessGpuThread {
 public:
  static base::Thread* Create(const InProcessChildThreadParams& params,
                              const gpu::GpuPreferences& gpu_preferences) {
    return new SlowGpuThread(params, gpu_preferences);
  }

  SlowGpuThread(const InProcessChildThreadParams& params,
                const gpu::GpuPreferences& gpu_preferences)
      : InProcessGpuThread(params, gpu_preferences),
        unpause_event_(base::WaitableEvent::ResetPolicy::MANUAL,
                       base::WaitableEvent::InitialState::NOT_SIGNALED) {
    slow_gpu_thread_created = true;
  }

  ~SlowGpuThread() override {
    // Here the IO thread signals the GPU thread to go on...
    unpause_event_.Signal();
    Stop();  // Do not destruct unpause_event_ until the GPU thread is done.
  }

 protected:
  void Init() override {
    // This GPU thread is stalled from its very beginning until its
    // destruction. This simulates an extreme but possible execution order.
    unpause_event_.Wait();
    InProcessGpuThread::Init();
  }

 private:
  WaitableEvent unpause_event_;
};

class InProcessGpuTest : public content::ContentBrowserTest {
 public:
  void SetUpCommandLine(base::CommandLine* command_line) override {
    command_line->AppendSwitch(switches::kInProcessGPU);
    content::ContentBrowserTest::SetUpCommandLine(command_line);
  }
};

class SlownInProcessGpuTest : public InProcessGpuTest {
 public:
  void SetUpCommandLine(base::CommandLine* command_line) override {
    // Avoid calls into GpuProcessHost::Get() until
    // we've specified it to use SlowGpuThread.
    command_line->AppendSwitch(switches::kDisableGpuEarlyInit);
    command_line->AppendSwitch(switches::kDisableGpuCompositing);
    InProcessGpuTest::SetUpCommandLine(command_line);
  }

  void SetUpOnMainThread() override {
    ASSERT_FALSE(slow_gpu_thread_created);
    content::RegisterGpuMainThreadFactory(SlowGpuThread::Create);
    InProcessGpuTest::SetUpOnMainThread();
  }
};

void CreateGpuProcessHost() {
  GpuProcessHost::Get();
}

void WaitUntilGpuProcessHostIsCreated() {
  base::RunLoop run_loop;
  content::BrowserThread::PostTaskAndReply(
      content::BrowserThread::IO, FROM_HERE,
      base::BindOnce(&CreateGpuProcessHost), run_loop.QuitClosure());
  run_loop.Run();
}

// Reproduces the race that could give crbug.com/799002's "hang until OOM" at
// shutdown.
IN_PROC_BROWSER_TEST_F(InProcessGpuTest, NoHangAtQuickLaunchAndShutDown) {
  ASSERT_FALSE(slow_gpu_thread_created);
  // ... then exit the browser.
}

// Tests crbug.com/799002 but with another timing.
IN_PROC_BROWSER_TEST_F(InProcessGpuTest, NoCrashAtShutdown) {
  WaitUntilGpuProcessHostIsCreated();
  ASSERT_FALSE(slow_gpu_thread_created);
  // ... then exit the browser.
}

// Reproduces the timing that caused ExtractInProcessMessagePipe() to crash.
// This bug got exposed after fixing crbug.com/799002's "hang until OOM".
IN_PROC_BROWSER_TEST_F(SlownInProcessGpuTest, NoCrashAtShutdown) {
  WaitUntilGpuProcessHostIsCreated();
  ASSERT_TRUE(slow_gpu_thread_created);
  // ... then exit the browser.
}

}  // namespace
