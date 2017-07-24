// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/crash/content/browser/child_process_crash_observer.h"

#include "base/bind.h"
#include "base/files/file_util.h"
#include "base/task_scheduler/post_task.h"
#include "components/crash/content/app/breakpad_linux.h"
#include "components/crash/content/browser/crash_dump_manager_android.h"

namespace breakpad {

ChildProcessCrashObserver::ChildProcessCrashObserver(
    const base::FilePath crash_dump_dir,
    int descriptor_id)
    : crash_dump_dir_(crash_dump_dir), descriptor_id_(descriptor_id) {}

ChildProcessCrashObserver::~ChildProcessCrashObserver() {}

void ChildProcessCrashObserver::OnChildStart(
    int child_process_id,
    content::FileDescriptorInfo* mappings) {
  if (breakpad::IsCrashReporterEnabled()) {
    base::PlatformFile file =
        CrashDumpManager::GetInstance()->CreateMinidumpForChild(
            child_process_id);
    if (file != base::kInvalidPlatformFile)
      mappings->Transfer(descriptor_id_, base::ScopedFD());
  }
}

void ChildProcessCrashObserver::OnChildExit(
    int child_process_id,
    base::ProcessHandle pid,
    content::ProcessType process_type,
    base::TerminationStatus termination_status,
    base::android::ApplicationState app_state) {
  base::PostTaskWithTraits(
      FROM_HERE, {base::MayBlock(), base::TaskPriority::BACKGROUND},
      base::Bind(&CrashDumpManager::ProcessMinidumpFromChild,
                 base::Unretained(CrashDumpManager::GetInstance()),
                 crash_dump_dir_, pid, process_type, termination_status,
                 app_state));
}

}  // namespace breakpad
