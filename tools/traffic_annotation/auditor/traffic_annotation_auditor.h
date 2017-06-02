// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TRAFFIC_ANNOTATION_AUDITOR_H_
#define TRAFFIC_ANNOTATION_AUDITOR_H_

#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/logging.h"

#include "tools/traffic_annotation/traffic_annotation.pb.h"

namespace traffic_annotation_auditor {

// Holds an instance of network traffic annotation.
class AnnotationInstance {
 public:
  AnnotationInstance();

  // Protobuf of the annotation.
  traffic_annotation::NetworkTrafficAnnotation proto;

  // Unique id of the annotation.
  std::string unique_id;

  // Extra id of the annotation (if available).
  std::string extra_id;

  // Annotation Type
  enum State {
    ANNOTATION_COMPLETE,
    ANNOTATION_PARTIAL,
    ANNOTATION_COMPLETENG,
    ANNOTATION_BRANCHED_COMPLETENG
  } state;
};

// Holds an instance of calling a function that might have a network traffic
// annotation argument.
struct CallInstance {
  CallInstance() : line_number(0), is_annotated(false) {}

  std::string file_path;
  std::string function_name;
  unsigned line_number;
  bool is_annotated;
};

class TrafficAnnotationAuditor {
 public:
  TrafficAnnotationAuditor(){};

  // Runs clang tool, traffic_annotation_extractor clang tool's outputs.
  std::string RunClangTool(const base::FilePath& source_path,
                           const base::FilePath& build_path,
                           const base::CommandLine::StringVector& path_filters,
                           const bool full_run);

  // Parses the output of clang tool and populates instances, calls, and errors.
  // Errors include not finding the file, or incorrect content.
  void ParsClangToolRawOutput(const std::string& clang_output,
                              std::vector<AnnotationInstance>* annotations,
                              std::vector<CallInstance>* calls,
                              std::vector<std::string>* errors);
};
}

#endif  // TRAFFIC_ANNOTATION_AUDITOR_H_