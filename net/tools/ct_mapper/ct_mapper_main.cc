// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <iostream>

#include "base/at_exit.h"
#include "base/command_line.h"
#include "base/compiler_specific.h"
#include "base/files/file_util.h"
#include "base/format_macros.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/stringprintf.h"
#include "net/tools/ct_mapper/entry_reader.h"
#include "net/tools/ct_mapper/mapper.h"
#include "net/tools/ct_mapper/metrics.h"
#include "net/tools/ct_mapper/my_visitor.h"
#include "net/tools/ct_mapper/write_results.h"

namespace net {
namespace {

const char kClobberSwitch[] = "clobber";
const char kNumThreadsSwitch[] = "threads";
const char kNumSamplesPerBucket[] = "samples";
const char kChunkSizeSwitch[] = "chunk_size";
const char kMaxElapsedSeconds[] = "max_elapsed_seconds";
const char kMaxPendingChunksSwitch[] = "max_pending_chunks";

const char kUsage[] =
    "Usage: ct_mapper [options] {inpath} {outpath}\n"
    "\n"
    "Iterates over all the certificate transparency entries in {inpath}. Each\n"
    "entry is categorized into zero or more buckets.\n"
    "\n"
    "  {inpath} Path to a directory containing a dump of the CT datbase\n"
    "           (can be created using dump-ct.sh)\n"
    "\n"
    "  {outpath} Directory where results will be written. The results\n"
    "            include an HTML file for exploring the data, as well \n"
    "            as samples from each bucket.\n"
    "\n"
    "OPTIONS\n"
    "\n"
    "  --threads={num}  Uses {num} threads to process all the data. "
    "Default=%" PRIuS
    ".\n"
    "\n"
    "  --samples={num}  Writes up to {num} samples for each bucket. "
    "Default=%" PRIuS
    ".\n"
    "\n"
    "  --chunk_size={num}  The smallest size of work that is consumed by a "
    "worker thread. It is a count of the number of entries. At one "
    "extreme the chunk size could be set to 1, in which case each worker "
    "thread can dequeue one entry at a time (which might not be the best "
    "for memory locality, or amount of locking overhead). Default=%" PRIuS
    ".\n"
    "\n"
    "  --max_elapsed_seconds={num}  The total amount of time to run before "
    "aborting.\n"
    "\n"
    "  --max_pending_chunks={num}  The maximum number of unconsumed chunks "
    "waiting to be grabbed by worker threads. Default=%" PRIuS ".\n";

// Helper to get a command line switch as an unsigned integer.
WARN_UNUSED_RESULT bool GetNumericSwitch(const base::CommandLine& command_line,
                                         const std::string& name,
                                         size_t* value,
                                         size_t default_value) {
  *value = default_value;

  std::string string_value = command_line.GetSwitchValueASCII(name);

  if (!string_value.empty()) {
    if (!base::StringToSizeT(string_value, value)) {
      std::cerr << "ERROR: --" << name << " must be an unsigned integer\n";
      return false;
    }
  }

  return true;
}

// Makes sure the input path is a directory, and looks like it contains CT
// entries.
bool GetInputPath(const base::CommandLine::StringType& value,
                  base::FilePath* path) {
  *path = base::FilePath(value);

  if (!base::PathExists(*path)) {
    std::cerr << "ERROR: input path doesn't exist: " << value << " \n";
    return false;
  }

  if (!base::DirectoryExists(*path)) {
    std::cerr << "ERROR: input path is not a directory: " << value << " \n";
    return false;
  }

  if (!base::PathExists(path->AppendASCII("entries.bin"))) {
    std::cerr << "ERROR: input directory does not contain an entries.bin file: "
              << value << " \n";
    return false;
  }

  if (!base::PathExists(path->AppendASCII("extra_certs.bin"))) {
    std::cerr
        << "ERROR: input directory does not contain an extra_certs.bin file: "
        << value << " \n";
    return false;
  }

  return true;
}

// Make sure the output path is a directory and create a new directory for it.
bool GetOutputPath(const base::CommandLine::StringType& value,
                   bool clobber,
                   base::FilePath* path) {
  // Skip output if special value "NONE" was given as the path.
  if (value == "NONE") {
    *path = base::FilePath();
    std::cerr << "Will not write output directory (because specified NONE)\n";
    return true;
  }

  // Create a temporary directory if special value "NEW" was given as the path.
  if (value == "NEW") {
    if (!base::CreateNewTempDirectory("ct_mapper", path)) {
      std::cerr << "ERROR: Couldn't create new (temp) output directory.\n";
      return false;
    }

    std::cerr << "Created output directory: " << path->value() << "\n";
    return true;
  }

  // Otherwise use the specified path.
  *path = base::FilePath(value);

  // Make sure the path doesn't already exist.
  if (base::PathExists(*path)) {
    if (clobber) {
      if (!DeleteFile(*path, true)) {
        std::cerr << "ERROR: Failed clobbering output path: " << value << "\n";
        return false;
      }
    } else {
      std::cerr
          << "ERROR: Output path already exists. If you want to overwrite "
             "it anyway, pass --"
          << kClobberSwitch
          << " (WARNING: this is equivalent to rm -rf): " << value << "\n";
      return false;
    }
  }

  if (!base::CreateDirectory(*path)) {
    std::cerr << "ERROR: Failed creating output directory: " << value << "\n";
    return false;
  }

  return true;
}

WARN_UNUSED_RESULT bool ParseCommandLine(
    const base::CommandLine& command_line,
    std::unique_ptr<EntryReader>* reader,
    std::unique_ptr<VisitorFactory>* visitor_factory,
    base::FilePath* output_dir,
    MapperOptions* options) {
  if (command_line.GetArgs().empty()) {
    std::cerr << "ERROR: Must specify input directory containing CT "
                 "entries.\n";
    return false;
  }

  if (command_line.GetArgs().size() < 2) {
    std::cerr
        << "ERROR: Must specify an output directory for the results. If "
           "you want to skip then use the string NONE. If you want to create a "
           "new directory use the string NEW\n";
    return false;
  }

  if (command_line.GetArgs().size() > 2) {
    std::cerr << "ERROR: Too many command line parameters (expecting 2)\n";
    return false;
  }

  base::FilePath input_path;

  if (!GetInputPath(command_line.GetArgs()[0], &input_path))
    return false;

  if (!GetOutputPath(command_line.GetArgs()[1],
                     command_line.HasSwitch(kClobberSwitch), output_dir)) {
    return false;
  }

  *reader = CreateEntryReaderForCertificateTransparencyDb(input_path);
  if (!(*reader))
    return false;

  MapperOptions default_options;

  if (!GetNumericSwitch(command_line, kNumThreadsSwitch, &options->num_threads,
                        default_options.num_threads)) {
    return false;
  }
  if (!GetNumericSwitch(command_line, kNumSamplesPerBucket,
                        &options->num_samples_per_bucket,
                        default_options.num_samples_per_bucket)) {
    return false;
  }
  if (!GetNumericSwitch(command_line, kChunkSizeSwitch, &options->chunk_size,
                        default_options.chunk_size)) {
    return false;
  }
  if (!GetNumericSwitch(command_line, kMaxPendingChunksSwitch,
                        &options->max_pending_chunks,
                        default_options.max_pending_chunks)) {
    return false;
  }

  size_t max_elapsed_seconds = 0;
  if (!GetNumericSwitch(command_line, kMaxElapsedSeconds, &max_elapsed_seconds,
                        0)) {
    return false;
  }
  options->max_elapsed_time = base::TimeDelta::FromSeconds(max_elapsed_seconds);

  // If no output directory was specified, no point capturing any samples.
  if (output_dir->empty()) {
    options->num_samples_per_bucket = 0;
  }

  *visitor_factory = CreateMyVisitorFactory();
  return *visitor_factory != nullptr;
}

std::string GetUsageString() {
  MapperOptions default_options;
  return base::StringPrintf(kUsage, default_options.num_threads,
                            default_options.num_samples_per_bucket,
                            default_options.chunk_size,
                            default_options.max_pending_chunks);
}

bool RunMain(const base::CommandLine& command_line) {
  std::unique_ptr<EntryReader> reader;
  std::unique_ptr<VisitorFactory> visitor_factory;
  base::FilePath output_dir;

  MapperOptions options;

  if (!ParseCommandLine(command_line, &reader, &visitor_factory, &output_dir,
                        &options)) {
    std::cerr << "\n" << GetUsageString();
    return false;
  }

  Metrics metrics;

  ForEachEntry(reader.get(), visitor_factory.get(), options, &metrics);

  if (!output_dir.empty()) {
    WriteResultsToDirectory(metrics, output_dir);
  }

  if (!output_dir.empty()) {
    std::cerr << "Wrote samples and overview to:\n"
              << output_dir.value() + "\n";
  }

  return true;
}

}  // namespace

}  // namespace net

int main(int argc, const char* argv[]) {
  base::AtExitManager at_exit_manager;
  base::CommandLine::Init(argc, argv);

  if (!net::RunMain(*base::CommandLine::ForCurrentProcess()))
    return -1;

  return 0;
}
