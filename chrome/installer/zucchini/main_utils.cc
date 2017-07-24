// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/installer/zucchini/main_utils.h"

#include "base/bind.h"
#include "base/command_line.h"
#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "base/process/process_metrics.h"
#include "build/build_config.h"
#include "chrome/installer/zucchini/io_utils.h"
#include "chrome/installer/zucchini/zucchini_commands.h"

namespace {

// Translates |command_line| arguments to a vector of base::FilePath and returns
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

}  // namespace

/******** ResourceUsageTracker ********/

bool ResourceUsageTracker::is_tracking_ = false;
std::unique_ptr<base::Time> ResourceUsageTracker::start_time_;

// static
void ResourceUsageTracker::Start() {
  start_time_ = base::MakeUnique<base::Time>(base::Time::Now());
  is_tracking_ = true;
}

// static
void ResourceUsageTracker::Finish() {
  if (!is_tracking_)
    return;

  base::Time end_time = base::Time::Now();

#if !defined(OS_MACOSX)
  std::unique_ptr<base::ProcessMetrics> process_metrics(
      base::ProcessMetrics::CreateProcessMetrics(
          base::GetCurrentProcessHandle()));

  LOG(INFO) << "Zucchini.PeakPagefileUsage "
            << process_metrics->GetPeakPagefileUsage() / 1024 << " KiB";
  LOG(INFO) << "Zucchini.PeakWorkingSetSize "
            << process_metrics->GetPeakWorkingSetSize() / 1024 << " KiB";
#endif  // !defined(OS_MACOSX)

  LOG(INFO) << "Zucchini.TotalTime " << (end_time - *start_time_).InSecondsF()
            << " s";
}

// static
void ResourceUsageTracker::Cancel() {
  start_time_.reset(nullptr);
  is_tracking_ = false;
}

/******** Command ********/

Command::Command(const char* name_in,
                 const char* usage_in,
                 int num_args_in,
                 Command::Fun fun_in)
    : name(name_in), usage(usage_in), num_args(num_args_in), fun(fun_in) {}

Command::Command(const Command&) = default;

Command::~Command() = default;

/******** CommandRegistry ********/

CommandRegistry::CommandRegistry(std::ostream* out) : out_(out) {}

CommandRegistry::~CommandRegistry() = default;

void CommandRegistry::RegisterAll() {
  commands_.push_back({"gen", "-gen <old_file> <new_file> <patch_file>",
                       command::kGenNumFileParams, base::Bind(command::Gen)});
  commands_.push_back({"apply", "-apply <old_file> <patch_file> <new_file>",
                       command::kApplyNumFileParams,
                       base::Bind(command::Apply)});
}

int CommandRegistry::Run(const base::CommandLine& command_line) {
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
    return zucchini::status::kStatusMissingCommand;
  }

  // Try to parse filename arguments. On failure, print usage and quit.
  std::vector<base::FilePath> fnames;
  if (!CheckAndGetFilePathParams(command_line, command_use->num_args,
                                 &fnames)) {
    *out_ << command_use->usage << std::endl;
    PrintUsage();
    return zucchini::status::kStatusMissingFiles;
  }

  did_run_command_ = true;
  return command_use->fun.Run(command_line, fnames);
}

void CommandRegistry::PrintUsage() {
  *out_ << "Usage:" << std::endl;
  for (const Command& command : commands_)
    *out_ << "  zucchini " << command.usage << std::endl;
}
