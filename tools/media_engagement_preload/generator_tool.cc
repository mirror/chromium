// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <iostream>

#include <map>
#include <set>
#include <string>
#include <vector>

#include "base/command_line.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/path_service.h"
#include "base/strings/utf_string_conversions.h"
#include "build/build_config.h"

#include "tools/media_engagement_preload/generator.h"

namespace {

// Print the command line help.
void PrintHelp() {
  std::cout << "generator_tool <json-file> <output-file> [--v=1]" << std::endl;
}

}  // namespace

int main(int argc, char* argv[]) {
  base::CommandLine::Init(argc, argv);
  const base::CommandLine& command_line =
      *base::CommandLine::ForCurrentProcess();

  logging::LoggingSettings settings;
  settings.logging_dest = logging::LOG_TO_SYSTEM_DEBUG_LOG;
  logging::InitLogging(settings);

#if defined(OS_WIN)
  std::vector<std::string> args;
  base::CommandLine::StringVector wide_args = command_line.GetArgs();
  for (const auto& arg : wide_args) {
    args.push_back(base::WideToUTF8(arg));
  }
#else
  base::CommandLine::StringVector args = command_line.GetArgs();
#endif
  if (args.size() != 2U) {
    PrintHelp();
    return 1;
  }

  // Check the JSON file exists.
  base::FilePath json_filepath = base::FilePath::FromUTF8Unsafe(argv[1]);
  if (!base::PathExists(json_filepath)) {
    LOG(ERROR) << "Input JSON file doesn't exist.";
    return 1;
  }
  json_filepath = base::MakeAbsoluteFilePath(json_filepath);

  // Read the JSON file.
  std::string json_input;
  if (!base::ReadFileToString(json_filepath, &json_input)) {
    LOG(ERROR) << "Could not read input JSON file.";
    return 1;
  }

  // Parse the JSON file.
  std::set<std::string> entries;
  if (!media::ParseJSON(json_input, &entries)) {
    LOG(ERROR) << "Error while parsing the input file.";
    return 1;
  }

  // Generate the Trie and store it in a proto.
  std::string output;
  output = media::GenerateToProto(entries);
  if (output.empty()) {
    LOG(ERROR) << "Trie generation failed.";
    return 1;
  }

  // Write the serialized proto to the output file.
  base::FilePath output_path;
  output_path = base::FilePath::FromUTF8Unsafe(argv[2]);
  if (base::WriteFile(output_path, output.c_str(),
                      static_cast<uint32_t>(output.size())) <= 0) {
    LOG(ERROR) << "Failed to write output.";
    return 1;
  }

  VLOG(1) << "Wrote trie containing " << entries.size() << " entries to "
          << output_path.AsUTF8Unsafe() << std::endl;

  return 0;
}
