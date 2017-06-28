// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/download/internal/stats.h"
#include "base/logging.h"

#include "components/download/internal/startup_status.h"

namespace download {
namespace stats {

std::string DownloadClientToString(DownloadClient& client) {
  switch (client) {
    case DownloadClient::INVALID:
      return "DownloadClient::INVALID";
    case DownloadClient::TEST:
      return "DownloadClient::TEST";
    case DownloadClient::TEST_2:
      return "DownloadClient::TEST_2";
    case DownloadClient::TEST_3:
      return "DownloadClient::TEST_3";
    case DownloadClient::OFFLINE_PAGE_PREFETCH:
      return "DownloadClient::OFFLINE_PAGE_PREFETCH";
    case DownloadClient::BOUNDARY:
      return "DownloadClient::BOUNDARY";
  }
  NOTREACHED();
  return "";
}

std::string ServiceApiActionToString(ServiceApiAction& action) {
  switch (action) {
    case ServiceApiAction::START_DOWNLOAD:
      return "ServiceApiAction::START_DOWNLOAD";
    case ServiceApiAction::PAUSE_DOWNLOAD:
      return "ServiceApiAction::PAUSE_DOWNLOAD";
    case ServiceApiAction::RESUME_DOWNLOAD:
      return "ServiceApiAction::RESUME_DOWNLOAD";
    case ServiceApiAction::CANCEL_DOWNLOAD:
      return "ServiceApiAction::CANCEL_DOWNLOAD";
    case ServiceApiAction::CHANGE_CRITERIA:
      return "ServiceApiAction::CHANGE_CRITERIA";
    case ServiceApiAction::COUNT:
      return "ServiceApiAction::COUNT";
  }
  NOTREACHED();
  return "";
}

void LogControllerStartupStatus(const StartupStatus& status) {
  DCHECK(status.Complete());

  // TODO(dtrainor): Log each failure reason.
  // TODO(dtrainor): Finally, log SUCCESS or FAILURE based on status.Ok().
}

void LogServiceApiAction(DownloadClient client, ServiceApiAction action) {
  LOG(ERROR) << DownloadClientToString(client) << " "
             << ServiceApiActionToString(action);
}

void LogStartDownloadResult(DownloadClient client,
                            DownloadParams::StartResult result) {
  // TODO(dtrainor): Log |result| for |client|.
}

void LogModelOperationResult(ModelAction action, bool success) {
  // TODO(dtrainor): Log |action|.
}

void LogScheduledTaskStatus(DownloadTaskType task_type,
                            ScheduledTaskStatus status) {
  // TODO(shaktisahu): Log |task_type| and |status|.
}

void LogDownloadCompletion(CompletionType type, unsigned int attempt_count) {
  // TODO(xingliu): Log completion.
}

void LogRecoveryOperation(Entry::State to_state) {
  // TODO(dtrainor): Log |to_state|.
}

void LogFileCleanupStatus(FileCleanupReason reason,
                          int attempted_cleanups,
                          int failed_cleanups,
                          int external_cleanups) {
  DCHECK_NE(FileCleanupReason::EXTERNAL, reason);
  // TODO(shaktisahu): Log |status| and |count|.
}

void LogFileDeletionFailed(int count) {
  // TODO(shaktisahu): Log |count|.
}

void LogFilePathsAreStrangelyDifferent() {
  // TODO(shaktisahu): Log this occurrence.
}

}  // namespace stats
}  // namespace download
