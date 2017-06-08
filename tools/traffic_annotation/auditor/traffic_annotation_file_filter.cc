// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "tools/traffic_annotation/auditor/traffic_annotation_file_filter.h"

#include <fstream>

#include "base/command_line.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/process/launch.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"

namespace {

// List of keywords that indicate a file may be related to network traffic
// annotations. This list includes all keywords related to defining annotations
// and all functions that need one.
const char* kRelevantKeywords[] = {
    "network_traffic_annotation",
    "network_traffic_annotation_test_helper",
    "NetworkTrafficAnnotationTag",
    "PartialNetworkTrafficAnnotationTag",
    "DefineNetworkTrafficAnnotation",
    "DefinePartialNetworkTrafficAnnotation",
    "CompleteNetworkTrafficAnnotation",
    "BranchedCompleteNetworkTrafficAnnotation",
    "NO_TRAFFIC_ANNOTATION_YET",
    "NO_PARTIAL_TRAFFIC_ANNOTATION_YET",
    "MISSING_TRAFFIC_ANNOTATION",
    "TRAFFIC_ANNOTATION_FOR_TESTS",
    "PARTIAL_TRAFFIC_ANNOTATION_FOR_TESTS",
    "SSLClientSocket",     // SSLClientSocket::
    "TCPClientSocket",     // TCPClientSocket::
    "UDPClientSocket",     // UDPClientSocket::
    "URLFetcher::Create",  // This one is used with class as it's too generic.
    "CreateDatagramClientSocket",   // ClientSocketFactory::
    "CreateSSLClientSocket",        // ClientSocketFactory::
    "CreateTransportClientSocket",  // ClientSocketFactory::
    "CreateRequest",                // URLRequestContext::
    nullptr                         // End Marker
};

}  // namespace

TrafficAnnotationFileFilter::TrafficAnnotationFileFilter() {
  file_name_matcher_ = std::regex(".*\\.(mm|cc)", std::regex::optimize);

  // Martin: Do you have any idea what is wrong with this? It does not match.
  std::regex keyword_checker("[A-Za-z:_]+", std::regex::optimize);
  std::string contents(".*(");
  for (int i = 0; kRelevantKeywords[i]; i++) {
    // DCHECK(std::regex_match(kRelevantKeywords[i], keyword_checker));
    contents = contents + kRelevantKeywords[i] + "|";
  }
  contents.pop_back();
  contents += ").*";
  content_matcher_ = std::regex(contents, std::regex::optimize);
}

TrafficAnnotationFileFilter::~TrafficAnnotationFileFilter() {}

void TrafficAnnotationFileFilter::GetFilesFromGit(
    const base::FilePath& source_path) {
#ifdef _WIN32
  const char* args[] = {"git.bat", "ls-files"};
#else
  const char* args[] = {"git", "ls-files"};
#endif
  base::CommandLine cmdline(2, args);

  // Change directory to source path to access git.
  base::FilePath original_path;
  base::GetCurrentDirectory(&original_path);
  base::SetCurrentDirectory(source_path);

  std::string results;
  if (base::GetAppOutput(cmdline, &results)) {
    for (const std::string file_path : base::SplitString(
             results, "\n", base::KEEP_WHITESPACE, base::SPLIT_WANT_ALL)) {
      if (IsFileRelevant(file_path))
        git_files_.push_back(file_path);
    }
  } else {
    LOG(ERROR) << "Could not get files from git.";
  }

  base::SetCurrentDirectory(original_path);
}

bool TrafficAnnotationFileFilter::IsFileRelevant(const std::string& file_path) {
  if (!std::regex_match(file_path, file_name_matcher_))
    return false;

  std::ifstream file_stream(file_path);
  std::string line;
  while (std::getline(file_stream, line)) {
    // MARTIN: Regex is 20X slower than string::find. Do you have any
    // suggestions or I just do it as it is?

    // if (std::regex_match(line, content_matcher_))
    //   return true;
    for (int i = 0; kRelevantKeywords[i]; i++) {
      if (line.find(kRelevantKeywords[i]) != std::string::npos)
        return true;
    }
  }
  // std::string file_content;
  // if (!base::ReadFileToString(
  //     base::FilePath(base::FilePath::StringPieceType(file_path)),
  //     &file_content)) {
  //   return false;
  // }
  // return std::regex_match(file_content, content_matcher_);
  return false;
}

void TrafficAnnotationFileFilter::GetRelevantFiles(
    const base::FilePath& source_path,
    const std::string directory_name,
    std::vector<std::string>* file_paths) {
  if (!git_files_.size())
    GetFilesFromGit(source_path);

  unsigned name_length = directory_name.length();
  for (const std::string& file_path : git_files_) {
    if (!strncmp(file_path.c_str(), directory_name.c_str(), name_length))
      file_paths->push_back(file_path);
  }
}
