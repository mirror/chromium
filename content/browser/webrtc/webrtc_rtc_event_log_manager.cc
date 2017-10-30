// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/webrtc/webrtc_rtc_event_log_manager.h"

#include <limits>

#include "base/bind.h"
#include "base/files/file_enumerator.h"
#include "base/files/file_util.h"  // TODO(eladalon): !!! Still used?
#include "base/memory/ptr_util.h"
#include "base/rand_util.h"
#include "base/task_scheduler/post_task.h"
#include "content/browser/webrtc/webrtc_internals.h"  // TODO(eladalon): !!! Remove
#include "content/public/browser/browser_thread.h"

// TODO(eladalon): !!! Why are multiple files always created in appr.tc?

// TODO(eladalon): !!! Run in debug mode.

// TODO(eladalon): !!! FilePath in place of std::string.

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
    //TODO(eladalon): !!! Replace by own path.
    : logs_base_path_(WebRTCInternals::GetInstance()->GetEventLogFilePath().DirName()),
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
  // TODO(eladalon): !!! Think about tear-down and flushing.
}

void RtcEventLogManager::RtcEventLogOpen(int render_process_id,
                                         int lid) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  // TODO(eladalon): !!! If the file fails to open, no real reason to keep
  // on wasting cycles with Write().
  task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&RtcEventLogManager::OpenLogFile,
                 weak_ptr_factory_.GetWeakPtr(), render_process_id, lid));
}

void RtcEventLogManager::RtcEventLogClose(int render_process_id,
                                          int lid) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
}

void RtcEventLogManager::OnRtcEventLogWrite(int render_process_id,
                                            int lid,
                                            const std::string& output) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&RtcEventLogManager::WriteToLogFile,
                 weak_ptr_factory_.GetWeakPtr(), render_process_id, lid, output));
}

void RtcEventLogManager::ProcessPendingLogsPath() {
  // TODO(eladalon): !!! Do I want to make sure that I run these on task_runner_?
  int types = base::FileEnumerator::FILES | base::FileEnumerator::DIRECTORIES;
#if defined(OS_POSIX)
  types |= base::FileEnumerator::SHOW_SYM_LINKS;
#endif

  // TODO(eladalon): !!! Use a filter to only enumerate over potential filenames?
  base::FileEnumerator base_dir(logs_base_path_, false, types);
  for (auto name = base_dir.Next(); !name.empty(); name = base_dir.Next()) {
    unavailable_filenames_.insert(name.BaseName().value());
  }
}

void RtcEventLogManager::OpenLogFile(int render_process_id, int lid) {
  // Find an unused filename for the new log file.
  std::string filename;
  while (true) {
    // TODO(eladalon): !!! Don't hard-code these.
    // TODO(eladalon): !!! Use proper extension-separator.
    // TODO(eladalon): !!! It would be nicer to have a fixed-length name.
    filename = "rtc_event_log_" + std::to_string(base::RandUint64()) + ".log";
    if (unavailable_filenames_.find(filename) == unavailable_filenames_.end()) {
      unavailable_filenames_.insert(filename);
      break;
    }
  }

  // Attempt to create the file.
  constexpr int file_flags = base::File::FLAG_CREATE |
                             base::File::FLAG_WRITE |
                             base::File::FLAG_EXCLUSIVE_WRITE;
  base::File file(logs_base_path_.Append(base::FilePath(filename)), file_flags);
  if (!file.IsValid()) {
    return;
  }

  // TODO(eladalon): !!! Any special Windows behavior necessary?

  // TODO(eladalon): !!! What can cause a valid file to become invalid?
  // If nothing, we should DCHECK it being valid. Otherwise, we should check
  // if it is.

  // If the file was successfully created, it's now ready to be written to.
  pc_to_log_file_mapping_.emplace(std::make_pair(
      PeerConnectionKey{render_process_id, lid}, std::move(std::move(file))));
}

void RtcEventLogManager::WriteToLogFile(int render_process_id,
                                        int lid,
                                        const std::string& output) {
  // TODO(eladalon): !!! Actually, can it be empty? This matters because, if it
  // can, writing it would close the file.
  CHECK(!output.empty());

  auto it =
      pc_to_log_file_mapping_.find(PeerConnectionKey{render_process_id, lid});
  // TODO(eladalon): !!! Validity - can we DCHECK on it?
  if (it == pc_to_log_file_mapping_.end() || !it->second.IsValid()) {
    return;
  }
  DCHECK_LE(output.length(), static_cast<size_t>(std::numeric_limits<int>::max()));
  // TODO(eladalon): !!! If bytes_written != output.length()...
//  int bytes_written = it->second.WriteAtCurrentPos(output.c_str(), output.length());
  it->second.WriteAtCurrentPos(output.c_str(), output.length());
}

}  // namespace content
