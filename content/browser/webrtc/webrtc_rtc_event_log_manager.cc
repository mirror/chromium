// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/webrtc/webrtc_rtc_event_log_manager.h"

#include <iostream>  // TODO(eladalon): !!! Remove.

#include "base/bind.h"
#include "base/files/file_enumerator.h"
#include "base/files/file_util.h"
#include "base/rand_util.h"
#include "base/task_scheduler/post_task.h"
#include "content/public/browser/browser_thread.h"

namespace content {

base::LazyInstance<RtcEventLogManager>::Leaky g_rtc_event_log_manager =
    LAZY_INSTANCE_INITIALIZER;

RtcEventLogManager* RtcEventLogManager::GetInstance() {
  return g_rtc_event_log_manager.Pointer();
}

// TODO(eladalon): !!! Use a 64bit random string as part of the path, to prevent
// name clashes. Alternatively, always scan for the next available path,
 // and keep the mapping in memory.
RtcEventLogManager::RtcEventLogManager()
    : base_file_path_("TODO(eladalon): !!!"),
      task_runner_(base::CreateSequencedTaskRunnerWithTraits(
          {base::MayBlock(), base::TaskPriority::BACKGROUND,
           base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN})),
      weak_ptr_factory_(this) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&RtcEventLogManager::ProcessPendingLogsPath,
                 weak_ptr_factory_.GetWeakPtr()));
}

RtcEventLogManager::~RtcEventLogManager() {
  // TODO(eladalon): !!! Do I perhaps want to guarantee that this also
  // happens on BrowserThread::UI?
}

void RtcEventLogManager::RtcEventLogOpen(int render_process_id,
                                         int lid) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  // TODO(eladalon): !!! If the file fails to open, no real reason to keep
  // on wasting cycles with Write().
  std::string path = GetFilePath(render_process_id, lid);
  task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&RtcEventLogManager::OpenLogFile,
                 weak_ptr_factory_.GetWeakPtr(), path));
}

void RtcEventLogManager::RtcEventLogClose(int render_process_id,
                                          int lid) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
}

void RtcEventLogManager::OnRtcEventLogWrite(int render_process_id,
                                            int lid,
                                            const std::string& output) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
}

std::string RtcEventLogManager::GetFilePath(int render_process_id,
                                            int lid) const {
  DCHECK_GE(render_process_id, 0);
  DCHECK_GE(lid, 0);
  return base_file_path_ + "_" + std::to_string(render_process_id) + "_" + std::to_string(lid);
}

void RtcEventLogManager::ProcessPendingLogsPath() {
  // TODO(eladalon): !!! Do I want to make sure that I run these on task_runner_?
  int types = base::FileEnumerator::FILES | base::FileEnumerator::DIRECTORIES;
#if defined(OS_POSIX)
  types |= base::FileEnumerator::SHOW_SYM_LINKS;
#endif

  base::FileEnumerator base_dir(base::FilePath(base_file_path_), false, types);
  for (auto name = base_dir.Next(); !name.empty(); name = base_dir.Next()) {
    std::cout << "Used: " << name.BaseName().value() << std::endl;
    unavailable_filenames_.insert(name.BaseName().value());
  }
}

void RtcEventLogManager::OpenLogFile(const std::string& path) {

}

}  // namespace content
