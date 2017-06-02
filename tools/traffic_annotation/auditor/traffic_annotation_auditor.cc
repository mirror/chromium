// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "tools/traffic_annotation/auditor/traffic_annotation_auditor.h"

#include "base/files/file_util.h"
#include "base/process/launch.h"
#include "base/strings/stringprintf.h"

namespace traffic_annotation_auditor {

AnnotationInstance::AnnotationInstance() : state(ANNOTATION_COMPLETE) {}

std::string TrafficAnnotationAuditor::RunClangTool(
    const base::FilePath& source_path,
    const base::FilePath& build_path,
    const base::CommandLine::StringVector& path_filters,
    const bool full_run) {
  base::CommandLine cmdline(source_path.Append(FILE_PATH_LITERAL("tools"))
                                .Append(FILE_PATH_LITERAL("clang"))
                                .Append(FILE_PATH_LITERAL("scripts"))
                                .Append(FILE_PATH_LITERAL("run_tool.py")));
  cmdline.AppendSwitch("generate-compdb");
  cmdline.AppendSwitch("tool=traffic_annotation_extractor");
  cmdline.AppendArg(
      base::StringPrintf("-p=%s", build_path.MaybeAsASCII().c_str()));

  if (full_run) {
    for (auto& path : path_filters)
      cmdline.AppendArg(path);
  } else {
    LOG(ERROR) << "NOT IMPLEMENTED YET";
    // if (!path_filters.size())
    //   path_filters.append(FILE_PATH_LITERAL(""));
  }

#ifdef _WIN32
  cmdline.PrependWrapper(L"python");
#endif

  std::string results;
  return base::GetAppOutput(cmdline, &results) ? results : "";
}

void TrafficAnnotationAuditor::ParsClangToolRawOutput(
    const std::string& clang_output,
    std::vector<AnnotationInstance>* annotations,
    std::vector<CallInstance>* calls,
    std::vector<std::string>* errors) {
  (*annotations)[0].state = AnnotationInstance::ANNOTATION_COMPLETE;
}
}
