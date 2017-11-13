// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/webrtc/webrtc_event_log_manager.h"

#include <inttypes.h>

#include <limits>

#include "base/bind.h"
#include "base/files/file_util.h"
#include "base/macros.h"
#include "base/strings/string_util.h"
#include "base/task_scheduler/post_task.h"
#include "content/public/browser/browser_thread.h"

// TODO(eladalon): !!! After the initial code-review iterations, but before
// landing this CL, add unit-tests.

namespace content {

base::LazyInstance<RtcEventLogManager>::Leaky g_rtc_event_log_manager =
    LAZY_INSTANCE_INITIALIZER;

RtcEventLogManager* RtcEventLogManager::GetInstance() {
  return g_rtc_event_log_manager.Pointer();
}

RtcEventLogManager::RtcEventLogManager()
    : clock_for_testing_(nullptr),
      file_task_runner_(base::CreateSequencedTaskRunnerWithTraits(
          {base::MayBlock(), base::TaskPriority::BACKGROUND,
           base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN})),
      weak_ptr_factory_(this) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
}

RtcEventLogManager::~RtcEventLogManager() {
  // This should never actually run, except for (potentially) in unit-tests.
}

void RtcEventLogManager::LocalRtcEventLogStart(int render_process_id,
                                               int lid,
                                               const base::FilePath& base_path,
                                               size_t max_file_size) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  file_task_runner_->PostTask(
      FROM_HERE, base::Bind(&RtcEventLogManager::StartLocalLogFile,
                            weak_ptr_factory_.GetWeakPtr(), render_process_id,
                            lid, base_path, max_file_size));
}

void RtcEventLogManager::LocalRtcEventLogStop(int render_process_id, int lid) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  file_task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&RtcEventLogManager::StopLocalLogFile,
                 weak_ptr_factory_.GetWeakPtr(), render_process_id, lid));
}

void RtcEventLogManager::OnRtcEventLogWrite(int render_process_id,
                                            int lid,
                                            const std::string& output) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  file_task_runner_->PostTask(
      FROM_HERE, base::Bind(&RtcEventLogManager::WriteToLocalLogFile,
                            weak_ptr_factory_.GetWeakPtr(), render_process_id,
                            lid, output));
  // TODO(eladalon): After the CL that takes care of remote-bound logs is
  // landed, we'll want to make sure that peer-connections that have neither
  // a remote-bound log nor a local-log-file associated, do not trigger
  // this callback, for efficiency's sake. (Files sometimes fail to be opened,
  // or reach their maximum size.)
}

void RtcEventLogManager::InjectClockForTesting(base::Clock* clock) {
  clock_for_testing_ = clock;
}

base::FilePath RtcEventLogManager::GetLocalFilePath(
    const base::FilePath& base_path,
    int render_process_id,
    int lid) {
  base::Time::Exploded now;
  if (clock_for_testing_) {
    clock_for_testing_->Now().LocalExplode(&now);
  } else {
    base::Time::Now().LocalExplode(&now);
  }

  // [user_defined]_[date]_[time]_[pid]_[lid].log
  char filename[100];
  int written = base::snprintf(filename, arraysize(filename),
                               "_%04d%02d%02d_%02d%02d_%d_%d", now.year,
                               now.month, now.day_of_month, now.hour,
                               now.minute, render_process_id, lid);
  if (0 < written && static_cast<size_t>(written) < arraysize(filename)) {
    return base_path.DirName().Append(base_path.BaseName().value() + filename);
  }

  return base::FilePath();
}

void RtcEventLogManager::StartLocalLogFile(int render_process_id,
                                           int lid,
                                           const base::FilePath& base_path,
                                           size_t max_file_size) {
  DCHECK(file_task_runner_->RunsTasksInCurrentSequence());

  // Add some information to the name given by the caller.
  base::FilePath file_path = GetLocalFilePath(base_path, render_process_id, lid)
                                 .AddExtension(FILE_PATH_LITERAL("log"));
  if (file_path.empty()) {
    return;
  }

  // In the unlikely case that this filename is already taken, find a unique
  // number to append to the filename, if possible.
  int unique_number =
      base::GetUniquePathNumber(file_path, FILE_PATH_LITERAL(""));
  if (unique_number < 0) {
    return;  // No available file path was found.
  } else if (unique_number != 0) {
    // The filename is taken, but a unique number was found.
    file_path = file_path.InsertBeforeExtension(FILE_PATH_LITERAL("_") +
                                                std::to_string(unique_number));
  }

  // Attempt to create the file.
  constexpr int file_flags = base::File::FLAG_CREATE | base::File::FLAG_WRITE |
                             base::File::FLAG_EXCLUSIVE_WRITE;
  base::File file(file_path, file_flags);
  if (!file.IsValid()) {
    return;
  }

  // If the file was successfully created, it's now ready to be written to.
  local_logs_.emplace(PeerConnectionKey{render_process_id, lid},
                      LogFile(std::move(file), max_file_size));
}

void RtcEventLogManager::StopLocalLogFile(int render_process_id, int lid) {
  DCHECK(file_task_runner_->RunsTasksInCurrentSequence());
  auto it = local_logs_.find(PeerConnectionKey{render_process_id, lid});
  if (it != local_logs_.end()) {
    CloseLocalLogFile(it);
  }
}

void RtcEventLogManager::CloseLocalLogFile(LocalLogFilesMap::iterator it) {
  DCHECK(file_task_runner_->RunsTasksInCurrentSequence());
  it->second.file.Flush();
  local_logs_.erase(it);  // file.Close() called by destructor.
}

void RtcEventLogManager::WriteToLocalLogFile(int render_process_id,
                                             int lid,
                                             const std::string& output) {
  DCHECK(file_task_runner_->RunsTasksInCurrentSequence());
  DCHECK_LE(output.length(),
            static_cast<size_t>(std::numeric_limits<int>::max()));

  if (output.empty()) {
    return;
  }

  auto it = local_logs_.find(PeerConnectionKey{render_process_id, lid});
  if (it == local_logs_.end()) {
    return;
  }

  // Observe the file size limit, if any. Note that base::File's interface does
  // not allow writing more than int::max() bytes at a time.
  int output_len = static_cast<int>(output.length());  // DCHECKed above.
  LogFile& log_file = it->second;
  if (log_file.max_file_size != kUnlimitedFileSize) {
    DCHECK_LT(log_file.file_size, log_file.max_file_size);
    if (log_file.file_size + output.length() < log_file.file_size ||
        log_file.file_size + output.length() > log_file.max_file_size) {
      output_len = log_file.max_file_size - log_file.file_size;
    }
  }

  int written = log_file.file.WriteAtCurrentPos(output.c_str(), output_len);
  if (written < 0 || written != output_len) {  // Error encountered.
    CloseLocalLogFile(it);
  }

  log_file.file_size += static_cast<size_t>(written);
  if (log_file.max_file_size != kUnlimitedFileSize) {
    DCHECK_LE(log_file.file_size, log_file.max_file_size);
    if (log_file.file_size >= log_file.max_file_size) {
      CloseLocalLogFile(it);
    }
  }
}

}  // namespace content
