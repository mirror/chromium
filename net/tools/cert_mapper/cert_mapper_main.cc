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
#include "net/tools/cert_mapper/entry_reader.h"
#include "net/tools/cert_mapper/entry_reader_ct.h"
#include "net/tools/cert_mapper/entry_reader_pem.h"
#include "net/tools/cert_mapper/mapper.h"
#include "net/tools/cert_mapper/metrics.h"
#include "net/tools/cert_mapper/my_visitor.h"
#include "net/tools/cert_mapper/write_results.h"

namespace net {
namespace {

const char kSwitchInPath[] = "in";
const char kSwitchInForm[] = "inform";
const char kSwitchOutPath[] = "out";
const char kSwitchClobber[] = "clobber";
const char kSwitchNumThreads[] = "threads";
const char kSwitchNumSamplesPerBucket[] = "samples";
const char kSwitchReadChunkSize[] = "read_chunk_size";
const char kSwitchMaxElapsedSeconds[] = "max_elapsed_seconds";

const char kUsage[] =
    "Usage: cert_mapper [options] --in=INPATH --inform=INFORM "
    "[--out=OUTPATH]\n"
    "\n"
    "Iterates over all the certificate \"entries\" in INPATH. Each\n"
    "entry is categorized into zero or more buckets.\n"
    "\n"
    "  INPATH:\n"
    "    Path that contains the certificate data. The structure\n"
    "    of this path depends on --inform: It could be a directory of PEM\n"
    "    files, or a binary dump of the Certificate Transparency database.\n"
    "\n"
    "  INFORM:\n"
    "    The type of data provided by INPATH. It can be either \"ct_bin\" or\n"
    "    \"tls_pem\"\n"
    "\n"
    "      * ct_bin: Means that INPATH is a directory containing a binary\n"
    "        dump of the Certificate Transparency database (created using\n"
    "        dump-ct.sh).\n"
    "\n"
    "      * tls_pem: Means that INPATH is a directory where each file is a\n"
    "        PEM file containing 1 or more \"CERTIFICATE\" blocks. This data\n"
    "        is expected to have come from TLS servers, where the first\n"
    "        certificate is the server certificate, and the rest are\n"
    "        possibly-unordered intermediates.\n"
    "\n"
    "  OUTPATH:\n"
    "    Optional path to a directory where the results and samples will be\n"
    "    written as an HTML report. The directory should not already exist\n"
    "    (if it   does you need to pass --clobber to overwrite it).\n"
    "\n"
    "Options\n"
    "\n"
    "  --threads={num}  Uses {num} threads to process all the data. "
    "Default=%" PRIuS
    ".\n"
    "\n"
    "  --samples={num}  Writes up to {num} samples for each bucket. "
    "Default=%" PRIuS
    ".\n"
    "\n"
    "  --read_chunk_size={num}  The smallest size of work that is consumed by "
    "a worker thread. It is a count of the number of entries. At one "
    "extreme the chunk size could be set to 1, in which case each worker "
    "thread can dequeue one entry at a time. Default=%" PRIuS
    ".\n"
    "\n"
    "  --max_elapsed_seconds={num}  The total amount of time to run before "
    "aborting. Useful for runnin over just a subset of the data.\n"
    "\n";

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

WARN_UNUSED_RESULT bool ParseCommandLine(
    const base::CommandLine& command_line,
    std::unique_ptr<EntryReader>* reader,
    std::unique_ptr<VisitorFactory>* visitor_factory,
    base::FilePath* output_dir,
    MapperOptions* options) {
  // ------------------------------------------
  // Process --in and --inform
  // ------------------------------------------

  if (!command_line.HasSwitch(kSwitchInPath)) {
    std::cerr << "ERROR: Must specify --" << kSwitchInPath << "\n";
    return false;
  }

  if (!command_line.HasSwitch(kSwitchInForm)) {
    std::cerr << "ERROR: Must specify --" << kSwitchInForm << "\n";
    return false;
  }

  base::FilePath in_path = command_line.GetSwitchValuePath(kSwitchInPath);
  std::string in_form = command_line.GetSwitchValueASCII(kSwitchInForm);

  if (in_form != "ct_bin" && in_form != "tls_pem") {
    std::cerr << "ERROR: Unknown value for --" << kSwitchInForm
              << ". Expecting either \"ct_bin\" or \"tls_pem\"\n";
    return false;
  }

  if (!base::PathExists(in_path)) {
    std::cerr << "ERROR: input path doesn't exist: " << in_path.value()
              << " \n";
    return false;
  }

  if (!base::DirectoryExists(in_path)) {
    std::cerr << "ERROR: input path is not a directory: " << in_path.value()
              << " \n";
    return false;
  }

  if (in_form == "ct_bin") {
    *reader = CreateEntryReaderForCertificateTransparencyDb(in_path);
  } else if (in_form == "tls_pem") {
    *reader = CreateEntryReaderForPemFiles(in_path);
  }

  if (!*reader) {
    std::cerr << "ERROR: failed initializing entry reader from input path: "
              << in_path.value() << "\n";
    return false;
  }

  // ------------------------------------------
  // Process --out and --clobber
  // ------------------------------------------

  if (command_line.HasSwitch(kSwitchOutPath)) {
    *output_dir = command_line.GetSwitchValuePath(kSwitchOutPath);
    bool clobber = command_line.HasSwitch(kSwitchClobber);

    if (base::PathExists(*output_dir)) {
      if (clobber) {
        if (!DeleteFile(*output_dir, true)) {
          std::cerr << "ERROR: Failed clobbering output path: "
                    << output_dir->value() << "\n";
          return false;
        }
      } else {
        std::cerr
            << "ERROR: Output path already exists. If you want to overwrite "
               "it anyway, pass --"
            << kSwitchClobber << " (WARNING: this is equivalent to rm -rf): "
            << output_dir->value() << "\n";
        return false;
      }
    }

    if (!base::CreateDirectory(*output_dir)) {
      std::cerr << "ERROR: Failed creating output directory: "
                << output_dir->value() << "\n";
      return false;
    }
  } else {
    *output_dir = base::FilePath();
  }

  // ------------------------------------------
  // Process the rest of the options.
  // ------------------------------------------

  if (!command_line.GetArgs().empty()) {
    std::cerr
        << "ERROR: Too many command line parameters (expecting only options)\n";
    return false;
  }

  MapperOptions default_options;

  if (!GetNumericSwitch(command_line, kSwitchNumThreads, &options->num_threads,
                        default_options.num_threads)) {
    return false;
  }
  if (!GetNumericSwitch(command_line, kSwitchNumSamplesPerBucket,
                        &options->num_samples_per_bucket,
                        default_options.num_samples_per_bucket)) {
    return false;
  }
  if (!GetNumericSwitch(command_line, kSwitchReadChunkSize,
                        &options->read_chunk_size,
                        default_options.read_chunk_size)) {
    return false;
  }

  size_t max_elapsed_seconds = 0;
  if (!GetNumericSwitch(command_line, kSwitchMaxElapsedSeconds,
                        &max_elapsed_seconds, 0)) {
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
                            default_options.read_chunk_size);
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

  if (!output_dir.empty()) {
    // Copy the static files to output directory first (useful in case git
    // checkout changes by the end of the map).
    CopyStaticFilesToDirectory(output_dir);
  }

  Metrics metrics;
  ForEachEntry(reader.get(), visitor_factory.get(), options, &metrics);

  if (!output_dir.empty()) {
    std::cerr
        << "\nWriting results to a directory (this can take a minute)...\n";
    WriteResultsToDirectory(metrics, output_dir);
    std::cerr << "Wrote results to: " << output_dir.value() << "\n";
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
