// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/optimization_guide/optimization_guide_service.h"

#include <string>

#include "base/bind.h"
#include "base/files/file.h"
#include "base/files/file_util.h"
#include "base/metrics/histogram_macros.h"
#include "base/strings/string_number_conversions.h"
#include "base/task_scheduler/post_task.h"

namespace optimization_guide {

namespace {

// Version "0" corresponds to no processed version. By service conventions,
// we represent it as a dotted triple.
const char kNullVersion[] = "0.0.0";

// Name of sentinel file that is created while processing the component
// data and removed when processing is completed (successful or not) that
// is meant to detect when processing is not completing (e.g., crashing).
// It holds a count of processing attempts (up to a limit).
const base::FilePath::CharType kSentinelFileName[] =
    FILE_PATH_LITERAL("process_attempt_sentinel.txt");

// Limit on number of attempts to fully process the component successfully
// (after which that component version will no longer be attempted).
const int kMaxProcessingAttempts = 3;

void RecordProcessHintsResult(
    OptimizationGuideService::ProcessHintsResult result) {
  UMA_HISTOGRAM_ENUMERATION(
      "OptimizationGuide.ProcessHintsResult", static_cast<int>(result),
      static_cast<int>(OptimizationGuideService::ProcessHintsResult::MAX));
}

}  // namespace

ComponentInfo::ComponentInfo(const base::Version& hints_version,
                             const base::FilePath& hints_path)
    : hints_version(hints_version), hints_path(hints_path) {}

ComponentInfo::~ComponentInfo() {}

OptimizationGuideService::OptimizationGuideService(
    const scoped_refptr<base::SingleThreadTaskRunner>& io_thread_task_runner)
    : background_task_runner_(base::CreateSequencedTaskRunnerWithTraits(
          {base::MayBlock(), base::TaskPriority::BACKGROUND})),
      io_thread_task_runner_(io_thread_task_runner),
      latest_processed_version_(kNullVersion) {
  DETACH_FROM_SEQUENCE(sequence_checker_);
}

OptimizationGuideService::~OptimizationGuideService() {}

void OptimizationGuideService::SetLatestProcessedVersionForTesting(
    const base::Version& version) {
  latest_processed_version_ = version;
}

void OptimizationGuideService::AddObserver(
    OptimizationGuideServiceObserver* observer) {
  if (io_thread_task_runner_->BelongsToCurrentThread()) {
    AddObserverOnIOThread(observer);
  } else {
    io_thread_task_runner_->PostTask(
        FROM_HERE, base::Bind(&OptimizationGuideService::AddObserverOnIOThread,
                              base::Unretained(this), observer));
  }
}

void OptimizationGuideService::AddObserverOnIOThread(
    OptimizationGuideServiceObserver* observer) {
  DCHECK(io_thread_task_runner_->BelongsToCurrentThread());
  observers_.AddObserver(observer);
}

void OptimizationGuideService::RemoveObserver(
    OptimizationGuideServiceObserver* observer) {
  if (io_thread_task_runner_->BelongsToCurrentThread()) {
    RemoveObserverOnIOThread(observer);
  } else {
    io_thread_task_runner_->PostTask(
        FROM_HERE,
        base::Bind(&OptimizationGuideService::RemoveObserverOnIOThread,
                   base::Unretained(this), observer));
  }
}

void OptimizationGuideService::RemoveObserverOnIOThread(
    OptimizationGuideServiceObserver* observer) {
  DCHECK(io_thread_task_runner_->BelongsToCurrentThread());
  observers_.RemoveObserver(observer);
}

void OptimizationGuideService::ProcessHints(
    const ComponentInfo& component_info) {
  background_task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&OptimizationGuideService::ProcessHintsInBackground,
                     base::Unretained(this), component_info));
}

bool OptimizationGuideService::CreateSentinelFile(
    const base::FilePath& sentinel_path) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  int attempt_count = 0;
  if (base::PathExists(sentinel_path)) {
    // Processing apparently did not complete previously, check attempt count.
    std::string content;
    if (!base::ReadFileToString(sentinel_path, &content)) {
      DLOG(ERROR) << "Error reading sentinel file";
    } else if (!base::StringToInt(content, &attempt_count)) {
      DLOG(ERROR) << "Error reading attempt count from sentinel file";
    } else {
      DLOG(WARNING) << "Processing component did not complete previously: "
                    << attempt_count << " attempts for " << sentinel_path;
    }
    if (attempt_count >= kMaxProcessingAttempts) {
      return false;
    }
  }

  // Set new attempt count in sentinel file.
  std::string new_sentinel_value = base::IntToString(++attempt_count);
  if (base::WriteFile(sentinel_path, new_sentinel_value.data(),
                      new_sentinel_value.length()) <= 0)
    DLOG(ERROR) << "Failed to create sentinel file " << sentinel_path;
  return true;
}

void OptimizationGuideService::DeleteSentinelFile(
    const base::FilePath& sentinel_path) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  if (base::DeleteFile(sentinel_path, false /* rescursive */))
    DLOG(ERROR) << "Error deleting sentinel file";
}

void OptimizationGuideService::ProcessHintsInBackground(
    const ComponentInfo& component_info) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  if (!component_info.hints_version.IsValid()) {
    RecordProcessHintsResult(ProcessHintsResult::FAILED_INVALID_PARAMETERS);
    return;
  }
  if (latest_processed_version_.CompareTo(component_info.hints_version) >= 0)
    return;
  if (component_info.hints_path.empty()) {
    RecordProcessHintsResult(ProcessHintsResult::FAILED_INVALID_PARAMETERS);
    return;
  }

  base::FilePath sentinel_path(
      component_info.hints_path.DirName().Append(kSentinelFileName));
  if (!CreateSentinelFile(sentinel_path)) {
    RecordProcessHintsResult(ProcessHintsResult::FAILED_FINISH_PROCESSING);
    return;
  }

  std::string binary_pb;
  if (!base::ReadFileToString(component_info.hints_path, &binary_pb)) {
    RecordProcessHintsResult(ProcessHintsResult::FAILED_READING_FILE);
    DeleteSentinelFile(sentinel_path);
    return;
  }

  proto::Configuration new_config;
  if (!new_config.ParseFromString(binary_pb)) {
    RecordProcessHintsResult(ProcessHintsResult::FAILED_INVALID_CONFIGURATION);
    DeleteSentinelFile(sentinel_path);
    return;
  }
  latest_processed_version_ = component_info.hints_version;

  RecordProcessHintsResult(ProcessHintsResult::SUCCESS);
  io_thread_task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&OptimizationGuideService::DispatchHintsOnIOThread,
                     base::Unretained(this), new_config, sentinel_path));
}

void OptimizationGuideService::DispatchHintsOnIOThread(
    const proto::Configuration& config,
    const base::FilePath& sentinel_path) {
  DCHECK(io_thread_task_runner_->BelongsToCurrentThread());

  for (auto& observer : observers_)
    observer.OnHintsProcessed(config);

  // Now that all observers had chance to parse the config without crashing,
  // clear the sentinel file on background thread.
  background_task_runner_->PostTask(
      FROM_HERE, base::BindOnce(&OptimizationGuideService::DeleteSentinelFile,
                                base::Unretained(this), sentinel_path));
}

}  // namespace optimization_guide
