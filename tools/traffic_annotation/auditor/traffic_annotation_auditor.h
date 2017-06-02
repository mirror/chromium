// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TRAFFIC_ANNOTATION_AUDITOR_H_
#define TRAFFIC_ANNOTATION_AUDITOR_H_

#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/logging.h"

namespace traffic_annotation {

class TrafficAnnotationAuditor {
 public:
  TrafficAnnotationAuditor(){};

  // Runs clang tool, traffic_annotation_extractor clang tool's outputs.
  std::string RunClangTool(const base::FilePath& source_path,
                           const base::FilePath& build_path,
                           const base::CommandLine::StringVector& path_filters,
                           const bool full_run);
};
}

#endif  // TRAFFIC_ANNOTATION_AUDITOR_H_