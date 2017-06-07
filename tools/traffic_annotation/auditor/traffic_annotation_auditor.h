// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TRAFFIC_ANNOTATION_AUDITOR_H_
#define TRAFFIC_ANNOTATION_AUDITOR_H_

#include "base/command_line.h"
#include "base/files/file_path.h"

#include "tools/traffic_annotation/traffic_annotation.pb.h"

namespace traffic_annotation_auditor {

// Holds an instance of network traffic annotation.
class AnnotationInstance {
 public:
  AnnotationInstance();
  AnnotationInstance(const AnnotationInstance& other);

  // Protobuf of the annotation.
  traffic_annotation::NetworkTrafficAnnotation proto;

  // Extra id of the annotation (if available).
  std::string extra_id;

  // Hash codes of unique id and extra id (if available).
  int unique_id_hash_code;
  int extra_id_hash_code;

  // Annotation Type
  enum AnnotationType {
    ANNOTATION_COMPLETE,
    ANNOTATION_PARTIAL,
    ANNOTATION_COMPLETENG,
    ANNOTATION_BRANCHED_COMPLETENG
  } annotation_type;

  // Deserializes an instance from serialized lines of text provided by the
  // clang tool.
  bool Deserialize(const std::vector<std::string>& serialized_lines,
                   int start_line,
                   int end_line,
                   std::string* error_text);
};

// Holds an instance of calling a function that might have a network traffic
// annotation argument.
class CallInstance {
 public:
  CallInstance();
  CallInstance(const CallInstance& other);

  std::string file_path;
  unsigned line_number;

  // Name of the function in which annotation is defined.
  std::string function_context;

  // Name of the function that may need annotation.
  std::string function_name;

  // Is function |function_name| annotated?
  bool is_annotated;

  // Deserializes an instance from serialized lines of text provided by the
  // clang tool.
  bool Deserialize(const std::vector<std::string>& serialized_lines,
                   int start_line,
                   int end_line,
                   std::string* error_text);
};

// Runs clang tool, traffic_annotation_extractor clang tool's outputs.
std::string RunClangTool(const base::FilePath& source_path,
                         const base::FilePath& build_path,
                         const base::CommandLine::StringVector& path_filters,
                         const bool full_run);

// Parses the output of clang tool and populates instances, calls, and errors.
// Errors include not finding the file, or incorrect content.
bool ParsClangToolRawOutput(const std::string& clang_output,
                            std::vector<AnnotationInstance>* annotations,
                            std::vector<CallInstance>* calls,
                            std::vector<std::string>* errors);

// Computes the hash value of a traffic annotation unique id.
int ComputeHashValue(const std::string& unique_id);
}

#endif  // TRAFFIC_ANNOTATION_AUDITOR_H_