// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "zucchini/main_utils.h"

#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <memory>

#include "base/files/file.h"
#include "base/files/file_util.h"
#include "base/files/memory_mapped_file.h"
#include "base/logging.h"
#include "base/process/launch.h"
#include "base/process/memory.h"
#include "base/process/process_metrics.h"

#include "zucchini/io_utils.h"
#include "zucchini/patch.h"
#include "zucchini/region.h"
#include "zucchini/stream.h"
#include "zucchini/zucchini.h"

namespace {

// Prints |message| and exits.
void Problem(const char* message) {
  std::cerr << message << std::endl;
  exit(1);
}

// Translates |command_line| arguments to a vector of base::FilePath and returns
// the result via |fnames|. Expects exactly |expected_count|.
bool CheckAndGetFilePathParams(const base::CommandLine& command_line,
                               size_t expected_count,
                               std::vector<base::FilePath>* fnames) {
  const base::CommandLine::StringVector& args = command_line.GetArgs();
  if (args.size() != expected_count)
    return false;

  fnames->clear();
  for (size_t i = 0; i < args.size(); ++i)
    fnames->push_back(base::FilePath(args[i]));
  return true;
}

/******** BufferedFileReader ********/

// A file reader that calls Problem() on failure.
class BufferedFileReader {
 public:
  explicit BufferedFileReader(const base::FilePath& file_name) {
    bool ok = buffer_.Initialize(file_name);
    if (!ok)  // This is also triggered if |file_name| is an empty file.
      Problem("Can't read file.");
  }
  uint8_t* data() { return buffer_.data(); }
  size_t length() const { return buffer_.length(); }
  zucchini::Region region() { return {data(), length()}; }

 private:
  base::MemoryMappedFile buffer_;
};

/******** BufferedFileWriter ********/

// A file writer that calls Problem() on failure.
class BufferedFileWriter {
 public:
  BufferedFileWriter(const base::FilePath& file_name, size_t length) {
    bool ok = buffer_.Initialize(
        {file_name, base::File::FLAG_CREATE_ALWAYS | base::File::FLAG_READ |
                        base::File::FLAG_WRITE},
        {0, int64_t(length)}, base::MemoryMappedFile::READ_WRITE_EXTEND);
    if (!ok)
      Problem("Can't create file.");
  }
  uint8_t* data() { return buffer_.data(); }
  size_t length() const { return buffer_.length(); }
  zucchini::Region region() { return {data(), length()}; }

 private:
  base::MemoryMappedFile buffer_;
};

}  // namespace

const char kSwitchDD[] = "dd";
const char kSwitchDump[] = "dump";
const char kSwitchImpose[] = "impose";
const char kSwitchKeep[] = "keep";
const char kSwitchQuiet[] = "quiet";
const char kSwitchRaw[] = "raw";

/******** ResourceUsageTracker ********/

ResourceUsageTracker::ResourceUsageTracker() : start_time_(base::Time::Now()) {}

void ResourceUsageTracker::Print() {
  base::Time end_time = base::Time::Now();
  std::unique_ptr<base::ProcessMetrics> process_metrics(
      base::ProcessMetrics::CreateProcessMetrics(
          base::GetCurrentProcessHandle()));

  std::cout << "Zucchini.PeakPagefileUsage "
            << process_metrics->GetPeakPagefileUsage() / 1024 << " KiB"
            << std::endl;
  std::cout << "Zucchini.PeakWorkingSetSize "
            << process_metrics->GetPeakWorkingSetSize() / 1024 << " KiB"
            << std::endl;
  std::cout << "Zucchini.TotalTime " << (end_time - start_time_).InSecondsF()
            << " s" << std::endl;
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

CommandRegistry::CommandRegistry() = default;

CommandRegistry::~CommandRegistry() = default;

void CommandRegistry::Register(const Command* command) {
  commands_.push_back(command);
}

void CommandRegistry::RunOrExit(const base::CommandLine& command_line) {
  const Command* command_use = nullptr;
  for (const Command* command : commands_) {
    if (command_line.HasSwitch(command->name)) {
      if (command_use) {        // Too many commands found.
        command_use = nullptr;  // Set to null to flag error.
        break;
      }
      command_use = command;
    }
  }

  // If we don't have exactly one matching command, print error and exit.
  if (!command_use) {
    std::cerr << "Must have exactly one of:\n  [";
    zucchini::PrefixSep sep(", ");
    for (const Command* command : commands_)
      std::cerr << sep << "-" << command->name;
    std::cerr << "]" << std::endl;
    PrintUsageAndExit();
  }

  std::vector<base::FilePath> fnames;
  if (CheckAndGetFilePathParams(command_line, command_use->num_args, &fnames)) {
    command_use->fun(command_line, fnames);
  } else {
    std::cerr << command_use->usage << std::endl;
    PrintUsageAndExit();
  }
}

void CommandRegistry::PrintUsageAndExit() {
  std::cerr << "Usage:" << std::endl;
  for (const Command* command : commands_)
    std::cerr << "  zucchini " << command->usage << std::endl;
  exit(1);
}

/******** Command Definitions ********/

Command kCommandGen = {
    "gen",
    "-gen <old_file> <new_file> <patch_file> [-raw]"  // Continues.
    " [-impose=#+#=#+#,#+#=#+#,...]",
    3,
    [](const base::CommandLine& command_line,
       const std::vector<base::FilePath>& fnames) {
      zucchini::PatchOptions opts;
      opts.force_raw = command_line.HasSwitch(kSwitchRaw);
      opts.imposed_matches = command_line.GetSwitchValueASCII(kSwitchImpose);

      BufferedFileReader old_image(fnames[0]);
      BufferedFileReader new_image(fnames[1]);
      zucchini::ZucchiniHeader header;
      header.magic = zucchini::ZucchiniHeader::kMagic;
      header.old_size = static_cast<uint32_t>(old_image.length());
      header.old_crc = old_image.region().crc32();
      header.new_size = static_cast<uint32_t>(new_image.length());
      header.new_crc = new_image.region().crc32();

      zucchini::SinkStreamSet patch_streams;
      if (!zucchini::Generate(opts, old_image.region(), new_image.region(),
                              &patch_streams)) {
        std::cout << "Fatal error found when generating patch." << std::endl;
        return;
      }

      // First pass: Dry run to measure total size.
      zucchini::SizeMeasurer measurer;
      zucchini::SinkStream<zucchini::SizeMeasurer::iterator> sink1 =
          zucchini::MakeSinkStream(measurer.begin());
      header.WriteTo(&sink1);
      patch_streams.Serialize(&sink1);

      // Second pass: Create output file with measured size, and write patch.
      BufferedFileWriter buffer(fnames[2], measurer.size());
      zucchini::SinkStream<uint8_t*> sink2 =
          zucchini::MakeSinkStream(buffer.data());
      header.WriteTo(&sink2);
      patch_streams.Serialize(&sink2);
    }};

Command kCommandApply = {
    "apply", "-apply <old_file> <patch_file> <new_file> [-keep]", 3,
    [](const base::CommandLine& command_line,
       const std::vector<base::FilePath>& fnames) {
      zucchini::ZucchiniHeader header;
      auto do_apply =
          [](const base::FilePath& old_file, const base::FilePath& patch_file,
             const base::FilePath& new_file, zucchini::ZucchiniHeader* header) {
            BufferedFileReader old_image(old_file);
            BufferedFileReader patch(patch_file);
            zucchini::SourceStream<uint8_t*> src =
                zucchini::MakeSourceStream(patch.region());

            if (!header->ReadFrom(&src))
              Problem("Error reading patch header.");
            if (header->magic != zucchini::ZucchiniHeader::kMagic)
              Problem("Invalid magic signature.");
            if (old_image.length() != header->old_size)
              Problem("Invalid old file (size mismatch).");
            if (header->old_crc != old_image.region().crc32())
              Problem("Invalid old file (CRC mismatch).");

            zucchini::SourceStreamSet patch_streams;
            if (!patch_streams.Init(&src))
              Problem("Invalid patch stream content.");

            BufferedFileWriter new_image(new_file, header->new_size);
            if (!zucchini::Apply(old_image.region(), &patch_streams,
                                 new_image.region())) {
              Problem("Fatal error found while applying patch.");
            }
          };

      auto do_check = [](const base::FilePath& new_file,
                         const zucchini::ZucchiniHeader& header,
                         bool keep_output_on_crc_failure) {
        if (BufferedFileReader(new_file).region().crc32() != header.new_crc) {
          if (keep_output_on_crc_failure) {
            Problem("Patch failure (CRC mismatch): Keeping bad output.");
          } else {
            base::DeleteFile(new_file, false);
            Problem("Patch failure (CRC mismatch): Output deleted (no -keep).");
          }
        }
      };

      do_apply(fnames[0], fnames[1], fnames[2], &header);
      do_check(fnames[2], header, command_line.HasSwitch(kSwitchKeep));
    }};

Command kCommandRead = {
    "read", "-read <exe> [-dump]", 1,
    [](const base::CommandLine& command_line,
       const std::vector<base::FilePath>& fnames) {
      BufferedFileReader input(fnames[0]);
      bool do_dump = command_line.HasSwitch(kSwitchDump);
      if (!zucchini::ReadRefs({input.data(), input.length()}, do_dump)) {
        std::cout << "Fatal error found when dumping pointers." << std::endl;
      }
    }};

Command kCommandDetect = {
    "detect", "-detect <archive_file> [-dd=format#]", 1,
    [](const base::CommandLine& command_line,
       const std::vector<base::FilePath>& fnames) {
      BufferedFileReader input(fnames[0]);
      std::vector<zucchini::Region> sub_image_list;
      if (!zucchini::DetectAll({input.data(), input.length()},
                               &sub_image_list)) {
        std::cout << "Fatal error found when detecting executables."
                  << std::endl;
      }

      std::string fmt = command_line.GetSwitchValueASCII(kSwitchDD);
      if (!fmt.empty() && !sub_image_list.empty()) {
        // Count max number of digits, for proper 0-padding to filenames.
        constexpr int kBufSize = 24;
        int digs = 1;  // Note: 10 -> 1 digit, since we'd output "0" to "9".
        for (size_t i = sub_image_list.size(); i > 10; i /= 10)
          ++digs;
        char buf[kBufSize];
        // Print dd commands that can be used to extract detected executables.
        std::cout << std::endl;
        std::cout << "*** Commands to extract detected files ***" << std::endl;
        for (size_t i = 0; i < sub_image_list.size(); ++i) {
          // Render format filename: Substitute # with |i| rendered to text.
          snprintf(buf, kBufSize, "%0*zu", digs, i);
          std::string filename;
          for (char ch : fmt) {
            if (ch == '#')
              filename += buf;
            else
              filename += ch;
          }
          zucchini::Region sub_image = sub_image_list[i];
          size_t skip = sub_image.begin() - input.data();
          size_t count = sub_image.size();
          std::cout << "dd bs=1 if=" << fnames[0].value();
          std::cout << " skip=" << skip << " count=" << count;
          std::cout << " of=" << filename << std::endl;
        }
      }
    }};

Command kCommandMatch = {
    "match", "-match <old_file> <new_file> [-impose=#+#=#+#,#+#=#+#,...]", 2,
    [](const base::CommandLine& command_line,
       const std::vector<base::FilePath>& fnames) {
      BufferedFileReader old_image(fnames[0]);
      BufferedFileReader new_image(fnames[1]);
      zucchini::PatchOptions opts;
      opts.imposed_matches = command_line.GetSwitchValueASCII(kSwitchImpose);
      if (!zucchini::MatchAll(opts, {old_image.data(), old_image.length()},
                              {new_image.data(), new_image.length()})) {
        std::cout << "Fatal error found when matching executables."
                  << std::endl;
      }
    }};

Command kCommandCrc32 = {
    "crc32", "-crc32 <file>", 1,
    [](const base::CommandLine& command_line,
       const std::vector<base::FilePath>& fnames) {
      BufferedFileReader image(fnames[0]);
      uint32_t crc = zucchini::Region(image.data(), image.length()).crc32();
      std::cout << zucchini::AsHex<8>(crc) << std::endl;
    }};
