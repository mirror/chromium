// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/installer/zucchini/main_utils.h"

#include <stddef.h>

#include <memory>

#include "base/command_line.h"
#include "base/logging.h"
#include "base/process/process_metrics.h"
#include "base/time/time.h"
#include "build/build_config.h"
#include "chrome/installer/zucchini/io_utils.h"

namespace {

// the result via |fnames|. Expects exactly |expected_count|.
bool CheckAndGetFilePathParams(const base::CommandLine& command_line,
                               size_t expected_count,
                               std::vector<base::FilePath>* fnames) {
  const base::CommandLine::StringVector& args = command_line.GetArgs();
  if (args.size() != expected_count)
    return false;

  fnames->clear();
  fnames->reserve(args.size());
  for (const auto& arg : args)
    fnames->emplace_back(arg);
  return true;
}

/******** ScopedResourceUsageTracker ********/

// A class to track and log system resource usage.
class ScopedResourceUsageTracker {
 public:
  // Initializes states for tracking.
  ScopedResourceUsageTracker() {
    start_time_ = base::TimeTicks::Now();

#if !defined(OS_MACOSX)
    std::unique_ptr<base::ProcessMetrics> process_metrics(
        base::ProcessMetrics::CreateProcessMetrics(
            base::GetCurrentProcessHandle()));
    start_peak_page_file_usage_ = process_metrics->GetPeakPagefileUsage();
    start_peak_working_set_size_ = process_metrics->GetPeakWorkingSetSize();
#endif  // !defined(OS_MACOSX)
  }

  // Computes and prints usage.
  ~ScopedResourceUsageTracker() {
    base::TimeTicks end_time = base::TimeTicks::Now();

#if !defined(OS_MACOSX)
    std::unique_ptr<base::ProcessMetrics> process_metrics(
        base::ProcessMetrics::CreateProcessMetrics(
            base::GetCurrentProcessHandle()));
    size_t cur_peak_page_file_usage = process_metrics->GetPeakPagefileUsage();
    size_t cur_peak_working_set_size = process_metrics->GetPeakWorkingSetSize();

    LOG(INFO) << "Zucchini.PeakPagefileUsage "
              << cur_peak_page_file_usage / 1024 << " KiB";
    LOG(INFO) << "Zucchini.PeakPagefileUsageChange "
              << (cur_peak_page_file_usage - start_peak_page_file_usage_) / 1024
              << " KiB";
    LOG(INFO) << "Zucchini.PeakWorkingSetSize "
              << cur_peak_working_set_size / 1024 << " KiB";
    LOG(INFO) << "Zucchini.PeakWorkingSetSizeChange "
              << (cur_peak_working_set_size - start_peak_working_set_size_) /
                     1024
              << " KiB";
#endif  // !defined(OS_MACOSX)

    LOG(INFO) << "Zucchini.TotalTime " << (end_time - start_time_).InSecondsF()
              << " s";
  }

 private:
  base::TimeTicks start_time_;
#if !defined(OS_MACOSX)
  size_t start_peak_page_file_usage_;
  size_t start_peak_working_set_size_;
#endif  // !defined(OS_MACOSX)
};

}  // namespace

/******** CommandRegistry ********/

CommandRegistry::CommandRegistry(std::ostream* out) : out_(out) {}

CommandRegistry::~CommandRegistry() = default;

void CommandRegistry::Add(const Command& command) {
  commands_.push_back(command);
}

zucchini::status::Code CommandRegistry::Run(
    const base::CommandLine& command_line) {
  const Command* command_use = nullptr;
  for (const Command& command : commands_) {
    if (command_line.HasSwitch(command.name)) {
      if (command_use) {        // Too many commands found.
        command_use = nullptr;  // Set to null to flag error.
        break;
      }
      command_use = &command;
    }
  }

  // If we don't have exactly one matching command, print usage and quit.
  if (!command_use) {
    *out_ << "Must have exactly one of:\n  [";
    zucchini::PrefixSep sep(", ");
    for (const Command& command : commands_)
      *out_ << sep << "-" << command.name;
    *out_ << "]" << std::endl;
    PrintUsage();
    return zucchini::status::kStatusInvalidParam;
  }

  // Try to parse filename arguments. On failure, print usage and quit.
  std::vector<base::FilePath> fnames;
  if (!CheckAndGetFilePathParams(command_line, command_use->num_args,
                                 &fnames)) {
    *out_ << command_use->usage << std::endl;
    PrintUsage();
    return zucchini::status::kStatusInvalidParam;
  }

  ScopedResourceUsageTracker resource_usage_tracker;
  return command_use->command_function(command_line, fnames);
}

void CommandRegistry::PrintUsage() {
  *out_ << "Usage:" << std::endl;
  for (const Command& command : commands_)
    *out_ << "  zucchini " << command.usage << std::endl;
}
