// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/download/internal/stats.h"

#include <map>

#include "base/files/file_enumerator.h"
#include "base/files/file_util.h"
#include "base/metrics/histogram_functions.h"
#include "base/metrics/histogram_macros.h"
#include "base/sys_info.h"
#include "base/threading/thread_restrictions.h"
#include "components/download/internal/startup_status.h"

namespace download {
namespace stats {
namespace {

// The maximum tracked file size in KB, larger files will fall into overflow
// bucket.
const int64_t kMaxFileSizeKB = 4 * 1024 * 1024;

// Converts DownloadTaskType to histogram suffix.
// Should maps to suffix string in histograms.xml.
std::string TaskTypeToHistogramSuffix(DownloadTaskType task_type) {
  switch (task_type) {
    case DownloadTaskType::DOWNLOAD_TASK:
      return "DownloadTask";
    case DownloadTaskType::CLEANUP_TASK:
      return "CleanUpTask";
  }
  NOTREACHED();
  return std::string();
}

// Converts Entry::State to histogram suffix.
// Should maps to suffix string in histograms.xml.
std::string EntryStateToHistogramSuffix(Entry::State state) {
  std::string suffix;
  switch (state) {
    case Entry::State::NEW:
      return "New";
    case Entry::State::AVAILABLE:
      return "Available";
    case Entry::State::ACTIVE:
      return "Active";
    case Entry::State::PAUSED:
      return "Paused";
    case Entry::State::COMPLETE:
      return "Complete";
    case Entry::State::MAX:
      break;
  }
  NOTREACHED();
  return std::string();
}

// Helper method to log StartUpResult. Should not changed to histogram macro
// here.
void LogStartUpResult(StartUpResult result) {
  base::UmaHistogramEnumeration("DownloadService.StartUpStatus", result,
                                StartUpResult::MAX);
}

// Helper method to log the number of entries under a particular state.
void LogDatabaseRecords(Entry::State state, uint32_t record_count) {
  std::string name("DownloadService.Db.Records");
  name.append(".").append(EntryStateToHistogramSuffix(state));
  base::UmaHistogramCustomCounts(name, record_count, 1, 500, 50);
}

}  // namespace

std::string ClientToHistogramSuffix(DownloadClient client) {
  std::string suffix;
  switch (client) {
    case DownloadClient::TEST:
    case DownloadClient::TEST_2:
    case DownloadClient::TEST_3:
    case DownloadClient::INVALID:
      break;
    case DownloadClient::OFFLINE_PAGE_PREFETCH:
      suffix = "OfflinePage";
    case DownloadClient::BOUNDARY:
      NOTREACHED();
      break;
  }
  return suffix;
}

void LogControllerStartupStatus(const StartupStatus& status) {
  DCHECK(status.Complete());

  // Total counts for general success/failure rate.
  LogStartUpResult(status.Ok() ? StartUpResult::SUCCESS
                               : StartUpResult::FAILURE);

  // Failure reasons.
  if (!status.driver_ok.value())
    LogStartUpResult(StartUpResult::FAILURE_REASON_DRIVER);
  if (!status.model_ok.value())
    LogStartUpResult(StartUpResult::FAILURE_REASON_MODEL);
  if (!status.file_monitor_ok.value())
    LogStartUpResult(StartUpResult::FAILURE_REASON_FILE_MONITOR);
}

void LogServiceApiAction(DownloadClient client, ServiceApiAction action) {
  // Total count for each action.
  std::string name("DownloadService.Request.ClientAction");
  base::UmaHistogramEnumeration(name, action, ServiceApiAction::MAX);

  // Total count for each action with client suffix.
  name.append(".").append(ClientToHistogramSuffix(client));
  base::UmaHistogramEnumeration(name, action, ServiceApiAction::MAX);
}

void LogStartDownloadResult(DownloadClient client,
                            DownloadParams::StartResult result) {
  // Total count for each start result.
  std::string name("DownloadService.Request.StartResult");
  base::UmaHistogramEnumeration(name, result, DownloadParams::StartResult::MAX);

  // Total count for each client result with client suffix.
  name.append(".").append(ClientToHistogramSuffix(client));
  base::UmaHistogramEnumeration(name, result, DownloadParams::StartResult::MAX);
}

void LogStartDownloadResponse(DownloadClient client,
                              Client::ShouldDownload should_download) {
  // Total count for each start result.
  std::string name("DownloadService.Request.StartResponse");
  base::UmaHistogramEnumeration(name, should_download,
                                Client::ShouldDownload::MAX);

  // Total count for each client result with client suffix.
  name.append(".").append(ClientToHistogramSuffix(client));
  base::UmaHistogramEnumeration(name, should_download,
                                Client::ShouldDownload::MAX);
}

void LogDownloadParams(const DownloadParams& params) {
  UMA_HISTOGRAM_ENUMERATION("DownloadService.Request.BatteryRequirement",
                            params.scheduling_params.battery_requirements,
                            SchedulingParams::BatteryRequirements::MAX);
  UMA_HISTOGRAM_ENUMERATION("DownloadService.Request.NetworkRequirement",
                            params.scheduling_params.network_requirements,
                            SchedulingParams::NetworkRequirements::MAX);
  UMA_HISTOGRAM_ENUMERATION("DownloadService.Request.Priority",
                            params.scheduling_params.priority,
                            SchedulingParams::Priority::MAX);
}

void LogRecoveryOperation(Entry::State to_state) {
  UMA_HISTOGRAM_ENUMERATION("DownloadService.Recovery", to_state,
                            Entry::State::MAX);
}

void LogDownloadCompletion(CompletionType type,
                           unsigned int attempt_count,
                           const base::TimeDelta& time_span) {
  UMA_HISTOGRAM_ENUMERATION("DownloadService.Download.Finish", type,
                            CompletionType::MAX);
  UMA_HISTOGRAM_COUNTS("DownloadService.Download.Retry", attempt_count);
}

void LogDownloadedFileSize(uint64_t file_size_bytes) {
  uint64_t file_size_kb = file_size_bytes / 1024;
  UMA_HISTOGRAM_CUSTOM_COUNTS("DownloadService.Finish.FileSize", file_size_kb,
                              1, kMaxFileSizeKB, 256);
}

void LogModelOperationResult(ModelAction action, bool success) {
  if (success) {
    UMA_HISTOGRAM_ENUMERATION("DownloadService.Db.Operation.Success", action,
                              ModelAction::MAX);
    return;
  }

  UMA_HISTOGRAM_ENUMERATION("DownloadService.Db.Operation.Failure", action,
                            ModelAction::MAX);
}

void LogEntries(std::map<Entry::State, uint32_t>& entries_count) {
  uint32_t total_records = 0;
  for (const auto& entry_count : entries_count)
    total_records += entry_count.second;

  // Total number of records in database.
  base::UmaHistogramCustomCounts("DownloadService.Db.Records", total_records, 1,
                                 500, 50);

  // Number of records for each Entry::State.
  LogDatabaseRecords(Entry::State::NEW, entries_count[Entry::State::NEW]);
  LogDatabaseRecords(Entry::State::AVAILABLE,
                     entries_count[Entry::State::AVAILABLE]);
  LogDatabaseRecords(Entry::State::ACTIVE, entries_count[Entry::State::ACTIVE]);
  LogDatabaseRecords(Entry::State::PAUSED, entries_count[Entry::State::PAUSED]);
  LogDatabaseRecords(Entry::State::COMPLETE,
                     entries_count[Entry::State::COMPLETE]);
}

void LogScheduledTaskStatus(DownloadTaskType task_type,
                            ScheduledTaskStatus status) {
  std::string name("DownloadService.TaskScheduler.Status");
  base::UmaHistogramEnumeration(name, status, ScheduledTaskStatus::MAX);

  name.append(".").append(TaskTypeToHistogramSuffix(task_type));
  base::UmaHistogramEnumeration(name, status, ScheduledTaskStatus::MAX);
}

void LogsFileDirectoryCreationError(base::File::Error error) {
  // Maps to histogram enum PlatformFileError.
  UMA_HISTOGRAM_ENUMERATION("DownloadService.Files.DirCreationError", -error,
                            -base::File::Error::FILE_ERROR_MAX);
}

void LogFileCleanupStatus(FileCleanupReason reason,
                          int attempted_cleanups,
                          int failed_cleanups,
                          int external_cleanups) {
  DCHECK_NE(FileCleanupReason::EXTERNAL, reason);
  UMA_HISTOGRAM_ENUMERATION("DownloadService.Files.CleanUpReason", reason,
                            FileCleanupReason::MAX);
  UMA_HISTOGRAM_COUNTS("DownloadService.Files.CleanUpAttempt",
                       attempted_cleanups);
  UMA_HISTOGRAM_COUNTS("DownloadService.Files.CleanUpFailure", failed_cleanups);
  UMA_HISTOGRAM_COUNTS("DownloadService.Files.CleanUpExternal",
                       external_cleanups);
}

void LogFileLifeTime(const base::TimeDelta& file_life_time) {
  UMA_HISTOGRAM_CUSTOM_TIMES("DownloadService.Files.LifeTime", file_life_time,
                             base::TimeDelta::FromSeconds(1),
                             base::TimeDelta::FromDays(8), 100);
}

void LogFileDirDiskUtilization(const base::FilePath& file_dir) {
  base::ThreadRestrictions::AssertIOAllowed();
  base::FileEnumerator file_enumerator(file_dir, false /* recursive */,
                                       base::FileEnumerator::FILES);

  int64_t total_size = 0, size = 0;
  // Compute the total size of all files in |file_dir|.
  for (base::FilePath path = file_enumerator.Next(); !path.value().empty();
       path = file_enumerator.Next()) {
    if (!base::GetFileSize(path, &size)) {
      DVLOG(1) << "File size query failed.";
      return;
    }
    total_size += size;
  }

  // Disk space of the volume that |file_dir| belongs to.
  int64_t total_disk_space = base::SysInfo::AmountOfTotalDiskSpace(file_dir);
  int64_t free_disk_space = base::SysInfo::AmountOfFreeDiskSpace(file_dir);
  if (total_disk_space == -1 || free_disk_space == -1) {
    DVLOG(1) << "System disk space query failed.";
    return;
  }

  if (total_disk_space == 0) {
    DVLOG(1) << "Empty total system disk space.";
    return;
  }

  // Record histograms.
  UMA_HISTOGRAM_PERCENTAGE("DownloadService.Files.FreeDiskSpace",
                           (free_disk_space * 100) / total_disk_space);
  UMA_HISTOGRAM_PERCENTAGE("DownloadService.Files.DiskUsed",
                           (total_size * 100) / total_disk_space);
  int64_t free_disk_without_files = free_disk_space + total_size;
  UMA_HISTOGRAM_PERCENTAGE("DownloadService.Files.FreeDiskUsed",
                           (free_disk_without_files == 0)
                               ? 0
                               : (total_size * 100) / free_disk_without_files);
}

void LogFilePathRenamed(bool renamed) {
  UMA_HISTOGRAM_BOOLEAN("DownloadService.Files.PathRenamed", renamed);
}

}  // namespace stats
}  // namespace download
