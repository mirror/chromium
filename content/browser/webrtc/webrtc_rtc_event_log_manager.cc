// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/webrtc/webrtc_rtc_event_log_manager.h"

#include "base/bind.h"
#include "base/files/file_util.h"
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
      weak_ptr_factory_(this) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);  // TODO(eladalon): !!! This?
}

RtcEventLogManager::~RtcEventLogManager() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);  // TODO(eladalon): !!! This?
}

void RtcEventLogManager::RtcEventLogOpen(int render_process_id,
                                         int local_pc_id) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  // TODO(eladalon): !!! If the file fails to open, no real reason to keep
  // on wasting cycles with Write().
  std::string path = GetFilePath(render_process_id, local_pc_id);
  base::PostTaskWithTraits(
      FROM_HERE, {base::MayBlock(), base::TaskPriority::BACKGROUND},
      base::Bind(&RtcEventLogManager::OpenLogFile, weak_ptr_factory_.GetWeakPtr(), path));
}

void RtcEventLogManager::RtcEventLogClose(int render_process_id,
                                          int local_pc_id) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
}

void RtcEventLogManager::OnRtcEventLogWrite(int render_process_id,
                                            int local_pc_id,
                                            const std::string& output) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
}

std::string RtcEventLogManager::GetFilePath(int render_process_id,
                                            int local_pc_id) const {
  DCHECK_GE(render_process_id, 0);
  DCHECK_GE(local_pc_id, 0);
  return base_file_path_ + "_" + std::to_string(render_process_id) + "_" + std::to_string(local_pc_id);
}

void RtcEventLogManager::OpenLogFile(const std::string& path) {

}

}  // namespace content
