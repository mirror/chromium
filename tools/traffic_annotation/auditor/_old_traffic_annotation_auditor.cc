// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// See README.md for information and build instructions.

#include "base/command_line.h"
#include "base/files/file_enumerator.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/logging.h"
#include "base/process/launch.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "third_party/protobuf/src/google/protobuf/text_format.h"

#include "tools/traffic_annotation/traffic_annotation.pb.h"

namespace {

// Holds an instance of network traffic annotation.
struct AnnotationInstance {
  AnnotationInstance() : state(STATE_OK) {}

  // Protobuf of the annotation.
  traffic_annotation::NetworkTrafficAnnotation proto;
  // Unique id of the annotation.
  std::string unique_id;
  // State of this annotation.
  enum State { STATE_OK, STATE_DUPLICATE, STATE_UNDEFINED } state;
};

// Reads an extracted annotation file and returns it in the output variables.
// The file starts with four |header_lines| with the following meaning:
//   0- File path.
//   1- Name of the function including this position.
//   2- Line number.
//   3- Unique id of annotation.
// The rest of the file is the protobuf text (|msg_text|).
bool ReadFile(const base::FilePath& file_path,
              std::vector<std::string>* header_lines,
              std::string* msg_text) {
  std::string file_content;
  if (!base::ReadFileToString(file_path, &file_content))
    return false;

  header_lines->clear();
  msg_text->clear();

  std::vector<std::string> tokens = base::SplitString(
      file_content, "\n", base::KEEP_WHITESPACE, base::SPLIT_WANT_ALL);

  // If enough data is extracted, populate outputs, otherwise leave them blank.
  if (tokens.size() > 4) {
    for (int i = 0; i < 4; i++)
      header_lines->push_back(tokens[i]);
    for (size_t i = 4; i < tokens.size(); i++)
      *msg_text += tokens[i] + "\n";
  }

  return true;
}

// Reads all extracted txt files from given input folder and populates instances
// and errors. Errors include not finding the file, incorrect content, or error
// passed from clang tool.
void ReadExtractedFiles(const base::FilePath& folder_name,
                        std::vector<AnnotationInstance>* instances,
                        std::vector<std::string>* errors) {
  base::FileEnumerator file_iter(folder_name, false,
                                 base::FileEnumerator::FILES,
                                 FILE_PATH_LITERAL("*.txt"));
  while (!file_iter.Next().empty()) {
    std::string file_name = file_iter.GetInfo().GetName().AsUTF8Unsafe();
    LOG(INFO) << "Reading " << file_name.c_str() << "...\n";

    std::vector<std::string> header_lines;
    std::string msg_text;
    if (!ReadFile(folder_name.Append(file_iter.GetInfo().GetName()),
                  &header_lines, &msg_text)) {
      errors->push_back(
          base::StringPrintf("Could not open file '%s'\n.", file_name.c_str()));
      continue;
    }
    if (header_lines.size() < 4) {
      errors->push_back(base::StringPrintf(
          "Header lines are not complete for file '%s'\n.", file_name.c_str()));
      continue;
    }

    // If annotation unique id is 'unittest', just ignore it.
    if (base::ToLowerASCII(header_lines[3]) == "unittest")
      continue;

    AnnotationInstance new_instance;

    if (header_lines[3] == "\"Undefined\"") {
      new_instance.state = AnnotationInstance::STATE_UNDEFINED;
    } else if (!google::protobuf::TextFormat::ParseFromString(
                   msg_text, (google::protobuf::Message*)&new_instance)) {
      errors->push_back(base::StringPrintf(
          "Could not parse protobuf for file '%s'\n.", file_name.c_str()));
      continue;
    }

    // Add header data to new instance.
    traffic_annotation::NetworkTrafficAnnotation_TrafficSource* src =
        new_instance.proto.mutable_source();
    src->set_file(header_lines[0]);
    src->set_function(header_lines[1]);
    int line;
    base::StringToInt(header_lines[2], &line);
    src->set_line(line);
    new_instance.unique_id = header_lines[3];
    instances->push_back(new_instance);
  }
  LOG(INFO) << instances->size() << " annotation instance(s) read.\n";
}

// Checks to see if unique ids are really unique and marks the ones which are
// not.
void MarkRepeatedUniqueIds(std::vector<AnnotationInstance>* instances) {
  std::map<std::string, AnnotationInstance*> unique_ids;
  for (auto& instance : *instances) {
    if (instance.state == AnnotationInstance::STATE_OK) {
      auto match = unique_ids.find(instance.unique_id);
      if (match != unique_ids.end()) {
        instance.state = match->second->state =
            AnnotationInstance::STATE_DUPLICATE;
      } else {
        unique_ids.insert(
            std::make_pair(std::string(instance.unique_id), &instance));
      }
    }
  }
}

// Writes summary of annotation instances to a file. Returns true if successful.
bool WriteSummaryFile(int clang_tool_exit_code,
                      const std::vector<AnnotationInstance>& instances,
                      const std::vector<std::string>& errors,
                      const base::FilePath& file_path) {
  std::string report = "";

  if (errors.size() || clang_tool_exit_code) {
    report = "[Errors]\n";

    if (clang_tool_exit_code)
      report += base::StringPrintf("Clang tool returned error: %i\n",
                                   clang_tool_exit_code);

    for (const auto& error : errors)
      report += error + "\n";
  }

  report += "[Annotations]\n";

  for (const auto& instance : instances) {
    report +=
        "------------------------------------------------------------"
        "--------------------\n";
    report += base::StringPrintf("Unique ID: %s\n", instance.unique_id.c_str());
    if (instance.state == AnnotationInstance::STATE_UNDEFINED)
      report += base::StringPrintf("WARNING: Undefined annotation.\n");
    else if (instance.state == AnnotationInstance::STATE_DUPLICATE)
      report += base::StringPrintf("WARNING: Duplicate unique id.\n");
    std::string temp;
    google::protobuf::TextFormat::PrintToString(instance.proto, &temp);
    report += base::StringPrintf("Content:\n%s\n", temp.c_str());
  }

  if (base::WriteFile(file_path, report.c_str(), report.length()) == -1) {
    LOG(INFO) << " Could not create output file: "
              << file_path.MaybeAsASCII().c_str() << ".\n";
    return false;
  }

  LOG(INFO) << "Output file " << file_path.MaybeAsASCII().c_str()
            << " written for " << instances.size() << " instances.\n";
  return true;
}

}  // namespace
