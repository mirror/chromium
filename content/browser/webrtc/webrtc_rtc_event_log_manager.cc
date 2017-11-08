// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/webrtc/webrtc_rtc_event_log_manager.h"

#include <inttypes.h>
#include <stdio.h>

#include <limits>

#include "base/bind.h"
#include "base/files/file_enumerator.h"
#include "base/files/file_util.h"  // TODO(eladalon): !!! Still used?
#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/rand_util.h"
#include "base/task_scheduler/post_task.h"
#include "content/public/browser/browser_thread.h"

namespace {

base::FilePath GetLocalFilePath(const base::FilePath& base_path,
                                int render_process_id,
                                int lid) {
  base::Time::Exploded now;
  base::Time::Now().LocalExplode(&now);

  // [user_defined]_[date]_[time]_[pid]_[lid].log
  char filename[100];
  int written =
      snprintf(filename, sizeof(filename), "_%04d%02d%02d_%02d%02d_%d_%d",
               now.year, now.month, now.day_of_month, now.hour, now.minute,
               render_process_id, lid);
  if (0 < written && static_cast<size_t>(written) < sizeof(filename)) {
    return base_path.DirName().Append(base_path.BaseName().value() + filename);
  }
  return base::FilePath();
}

}  // namespace

namespace content {

base::LazyInstance<RtcEventLogManager>::Leaky g_rtc_event_log_manager =
    LAZY_INSTANCE_INITIALIZER;

RtcEventLogManager* RtcEventLogManager::GetInstance() {
  return g_rtc_event_log_manager.Pointer();
}

RtcEventLogManager::RtcEventLogManager()
    : task_runner_(base::CreateSequencedTaskRunnerWithTraits(
          {base::MayBlock(), base::TaskPriority::BACKGROUND,
           base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN})),
      weak_ptr_factory_(this) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
}

RtcEventLogManager::~RtcEventLogManager() {
  // TODO(eladalon): !!! Do I perhaps want to guarantee that this also
  // happens on BrowserThread::UI?
  // TODO(eladalon): !!! Think about tear-down and flushing.
}

void RtcEventLogManager::LocalRtcEventLogStart(
    int render_process_id,
    int lid,
    const base::FilePath& base_path) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  // Known edge case - if the file fails to be opened, and if there's no
  // remote-bound log file associated with the same PC, then the calls to
  // WriteToLocalLogFile() for this PC will be wasted. This is too edgy to care
  // about the slight performance improvement to be had.
  task_runner_->PostTask(
      FROM_HERE, base::Bind(&RtcEventLogManager::StartLocalLogFile,
                            weak_ptr_factory_.GetWeakPtr(), render_process_id,
                            lid, base_path));
}

// TODO(eladalon): !!! This was not called when I hung up appr.tc. However,
// it could be that it just didn't close the PC.
void RtcEventLogManager::LocalRtcEventLogStop(int render_process_id, int lid) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&RtcEventLogManager::StopLocalLogFile,
                 weak_ptr_factory_.GetWeakPtr(), render_process_id, lid));
}

void RtcEventLogManager::OnRtcEventLogWrite(int render_process_id,
                                            int lid,
                                            const std::string& output) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  task_runner_->PostTask(
      FROM_HERE, base::Bind(&RtcEventLogManager::WriteToLocalLogFile,
                            weak_ptr_factory_.GetWeakPtr(), render_process_id,
                            lid, output));
}

void RtcEventLogManager::StartLocalLogFile(int render_process_id,
                                           int lid,
                                           const base::FilePath& base_path) {
  DCHECK(task_runner_->RunsTasksInCurrentSequence());

  // Add some information to the name given by the caller.
  base::FilePath file_path =
      GetLocalFilePath(base_path, render_process_id, lid).AddExtension("log");
  if (file_path.empty()) {
    return;
  }

  // In the unlikely case that this filename is already taken, find a unique
  // number to append to the filename, if possible.
  int unique_number = base::GetUniquePathNumber(file_path, "");
  if (unique_number < 0) {
    return;  // No available file path was found.
  } else if (unique_number != 0) {
    // The filename is taken, but a unique number was found.
    file_path =
        file_path.InsertBeforeExtension("_" + std::to_string(unique_number));
  }

  // Attempt to create the file.
  constexpr int file_flags = base::File::FLAG_CREATE | base::File::FLAG_WRITE |
                             base::File::FLAG_EXCLUSIVE_WRITE;
  base::File file(file_path, file_flags);
  if (!file.IsValid()) {
    return;
  }

  // TODO(eladalon): !!! Any special Windows behavior necessary?

  // If the file was successfully created, it's now ready to be written to.
  pc_to_local_log_.emplace(std::make_pair(
      PeerConnectionKey{render_process_id, lid}, std::move(std::move(file))));
}

void RtcEventLogManager::StopLocalLogFile(int render_process_id, int lid) {
  DCHECK(task_runner_->RunsTasksInCurrentSequence());
  // TODO(eladalon): !!! Implement.
}

void RtcEventLogManager::WriteToLocalLogFile(int render_process_id,
                                             int lid,
                                             const std::string& output) {
  DCHECK(task_runner_->RunsTasksInCurrentSequence());
  DCHECK_LE(output.length(),
            static_cast<size_t>(std::numeric_limits<int>::max()));
  // TODO(eladalon): !!! If this is possible, ensure that it will be stopped
  // earlier.
  DCHECK(!output.empty());

  auto it = pc_to_local_log_.find(PeerConnectionKey{render_process_id, lid});
  if (it == pc_to_local_log_.end()) {
    return;
  }
  if (!it->second.IsValid()) {
    pc_to_local_log_.erase(it);
    return;
  }

  // TODO(eladalon): !!! If bytes_written != output.length()...
  it->second.WriteAtCurrentPos(output.c_str(), output.length());
}

}  // namespace content
