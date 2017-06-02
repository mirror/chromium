// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "tools/traffic_annotation/auditor/traffic_annotation_auditor.h"

auto* HELP_TEXT = R"(
Traffic Annotation Auditor
Extracts network traffic annotaions from the repository, audits them for errors
and coverage, and produces reports.

Usage: traffic_annotation_auditor [OPTION]... [path_filters]

Extracts network traffic annotations from source files. If path filter(s) are
specified, only those directories of the source  will be analyzed.

Options:
  -h, --help          Shows help.
  --build-path        Path to the build directory.
  --extractor-output  Optional path to the temporary file that extracted
                      annotations will be stored into.
  --extracted-input   Optional path to the file that temporary extracted
                      annotations are already stored in. If this is provided,
                      clang tool is not run and this is used as input.
  --full-run          Optional flag asking the tool to run on the whole
                      repository without text filtering files. Using this flag
                      may increase processing time x40.
  --summary-file      Optional path to the output file with all annotations.
  --ids-file          Optional path to the output file with the list of unique
                      ids and their hash codes.
  path_filters        Optional paths to filter what files the tool is run on.

Example:
  traffic_annotation_auditor --build-dir=out/Debug summary-file=report.txt
)";

#ifdef _WIN32
int wmain(int argc, wchar_t* argv[]) {
#else
int main(int argc, char* argv[]) {
#endif
  // Parse switches.
  base::CommandLine command_line = base::CommandLine(argc, argv);
  if (command_line.HasSwitch("help") || command_line.HasSwitch("h")) {
    LOG(INFO) << HELP_TEXT;
    return 1;
  }

  // FILE_PATH_LITERAL?
  const base::FilePath build_path =
      command_line.GetSwitchValuePath("build-path");
  const base::FilePath extractor_output =
      command_line.GetSwitchValuePath("extractor-output");
  const base::FilePath extractor_input =
      command_line.GetSwitchValuePath("extractor-input");
  bool full_run = command_line.HasSwitch("full-run");
  const base::FilePath summary_file =
      command_line.GetSwitchValuePath("summary-file");
  const base::FilePath ids_file = command_line.GetSwitchValuePath("ids-file");
  const base::CommandLine::StringVector path_filters = command_line.GetArgs();

  traffic_annotation::TrafficAnnotationAuditor auditor;

  // Extract annotations.
  std::string raw_annotations;
  if (extractor_input.empty()) {
    // Get build directory, if it is empty issue an error.
    if (build_path.empty()) {
      LOG(ERROR)
          << "You must either specify the build directory to run the clang "
             "tool and extract annotations, or specify the input file where "
             "extracted annotations already exist.\n";
      return 1;
    }

    // Eexcutable is usually in out/[Build Dir], so the path to source is
    // extracted by moving two directories up.
    base::FilePath source_path =
        command_line.GetProgram().DirName().Append("..").Append("..");

    raw_annotations =
        auditor.RunClangTool(source_path, build_path, path_filters, full_run);
  } else {
    // Read extracted annotations from file.
    raw_annotations = "";
  }

  // // Read all extracted files.
  // std::vector<AnnotationInstance> instances;
  // std::vector<std::string> errors;
  // ReadExtractedFiles(extracted_files_dir, &instances, &errors);

  // if (instances.empty()) {
  //   LOG(ERROR) << "Could not read any file.\n";
  //   return 1;
  // } else {
  //   MarkRepeatedUniqueIds(&instances);
  // }

  // // Create Summary file if requested.
  // if (!summary_file.empty() &&
  //     !WriteSummaryFile(clang_tool_exit_code, instances, errors,
  //     summary_file))
  //   return 1;

  return 0;
}