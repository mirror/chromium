// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/profiling_host/background_profiling_triggers.h"

#include "base/task_scheduler/post_task.h"
#include "chrome/browser/profiling_host/profiling_process_host.h"
#include "content/public/browser/browser_thread.h"
#include "services/resource_coordinator/public/cpp/memory_instrumentation/memory_instrumentation.h"

namespace profiling {

namespace {
// Check memory usage every 5 minutes. Trigger slow report if needed.
const int kRepeatingCheckMemoryDelayInSeconds = 5 * 60;
const size_t kBrowserProcessMallocTriggerKb = 800 * 1024;    // 800 Meg
const size_t kRendererProcessMallocTriggerKb = 2000 * 1024;  // 2 Gig
}  // namespace

BackgroundProfilingTriggers::BackgroundProfilingTriggers() {
  // Register a repeating timer to check memory usage periodically.
  timer_.Start(
      FROM_HERE,
      base::TimeDelta::FromSeconds(kRepeatingCheckMemoryDelayInSeconds),
      base::Bind(&BackgroundProfilingTriggers::PerformMemoryUsageChecks,
                 base::Unretained(this)));
}

BackgroundProfilingTriggers::~BackgroundProfilingTriggers() {}

void BackgroundProfilingTriggers::PerformMemoryUsageChecks() {
  content::BrowserThread::PostTask(
      content::BrowserThread::IO, FROM_HERE,
      base::BindOnce(&BackgroundProfilingTriggers::
                         PerformMemoryUsageChecksOnBlockingThread,
                     base::Unretained(this)));
}

void BackgroundProfilingTriggers::PerformMemoryUsageChecksOnBlockingThread() {
  memory_instrumentation::MemoryInstrumentation::GetInstance()
      ->RequestGlobalDump(
          base::Bind(&BackgroundProfilingTriggers::OnReceivedMemoryDump,
                     base::Unretained(this)));
}

void BackgroundProfilingTriggers::OnReceivedMemoryDump(
    bool success,
    memory_instrumentation::mojom::GlobalMemoryDumpPtr dump) {
  if (!success)
    return;

  DCHECK_NE(profiling::ProfilingProcessHost::GetCurrentMode(),
            profiling::ProfilingProcessHost::Mode::kNone);

  for (const auto& proc : dump->process_dumps) {
    bool trigger_report = false;
    if (proc->process_type ==
        memory_instrumentation::mojom::ProcessType::BROWSER) {
      trigger_report =
          proc->chrome_dump->malloc_total_kb > kBrowserProcessMallocTriggerKb;
    }

    if (profiling::ProfilingProcessHost::GetCurrentMode() ==
        profiling::ProfilingProcessHost::Mode::kAll) {
      if (proc->process_type ==
          memory_instrumentation::mojom::ProcessType::RENDERER) {
        trigger_report = proc->chrome_dump->malloc_total_kb >
                         kRendererProcessMallocTriggerKb;
      }
    }

    if (trigger_report) {
      int pid = proc->pid;
      profiling::ProfilingProcessHost::GetInstance()->RequestProcessReport(pid);

      // Do not execute more tasks to avoid uploading too many reports.
      timer_.Stop();
    }
  }
}

}  // namespace profiling
