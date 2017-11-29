// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "tools/traffic_annotation/auditor/traffic_annotation_file_filter.h"

#include <fstream>

#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/process/launch.h"
#include "base/stl_util.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "build/build_config.h"

namespace {

// List of keywords that indicate a file may be related to network traffic
// annotations. This list includes all keywords related to defining annotations
// and all functions that need one.
const char* kRelevantKeywords[] = {
    "network_traffic_annotation", "network_traffic_annotation_test_helper",
    "NetworkTrafficAnnotationTag", "PartialNetworkTrafficAnnotationTag",
    "DefineNetworkTrafficAnnotation", "DefinePartialNetworkTrafficAnnotation",
    "CompleteNetworkTrafficAnnotation",
    "BranchedCompleteNetworkTrafficAnnotation", "NO_TRAFFIC_ANNOTATION_YET",
    "NO_PARTIAL_TRAFFIC_ANNOTATION_YET", "MISSING_TRAFFIC_ANNOTATION",
    "TRAFFIC_ANNOTATION_FOR_TESTS", "PARTIAL_TRAFFIC_ANNOTATION_FOR_TESTS",
    //    "SSLClientSocket",     // SSLClientSocket::
    //   "TCPClientSocket",     // TCPClientSocket::
    //    "UDPClientSocket",     // UDPClientSocket::
    //    "URLFetcher::Create",  // This one is used with class as it's too
    //    generic. "CreateDatagramClientSocket",   // ClientSocketFactory::
    //    "CreateSSLClientSocket",        // ClientSocketFactory::
    //    "CreateTransportClientSocket",  // ClientSocketFactory::
    "CreateRequest",  // URLRequestContext::
    nullptr           // End Marker
};

}  // namespace

TrafficAnnotationFileFilter::TrafficAnnotationFileFilter() {}

TrafficAnnotationFileFilter::~TrafficAnnotationFileFilter() {}

void TrafficAnnotationFileFilter::GetFilesFromGit(
    const base::FilePath& source_path) {
  // Change directory to source path to access git and check files.
  base::FilePath original_path;
  base::GetCurrentDirectory(&original_path);
  base::SetCurrentDirectory(source_path);

  std::string git_list;
  if (git_file_for_test_.empty()) {
    const base::CommandLine::CharType* args[] =
#if defined(OS_WIN)
        {FILE_PATH_LITERAL("git.bat"), FILE_PATH_LITERAL("ls-files")};
#else
        {"git", "ls-files"};
#endif
    base::CommandLine cmdline(2, args);

    // Get list of files from git.
    if (!base::GetAppOutput(cmdline, &git_list)) {
      LOG(ERROR) << "Could not get files from git.";
      git_list.clear();
    }
  } else {
    if (!base::ReadFileToString(git_file_for_test_, &git_list)) {
      LOG(ERROR) << "Could not load mock git list file from "
                 << git_file_for_test_.MaybeAsASCII().c_str();
      git_list.clear();
    }
  }

  for (const std::string file_path : base::SplitString(
           git_list, "\n", base::KEEP_WHITESPACE, base::SPLIT_WANT_ALL)) {
    if (IsFileRelevant(file_path))
      git_files_.push_back(file_path);
  }

  base::SetCurrentDirectory(original_path);
}

bool TrafficAnnotationFileFilter::IsFileRelevant(const std::string& file_path) {
  // Check file extension.
  int pos = file_path.length() - 3;

  if (pos < 0 || (strcmp(".mm", file_path.c_str() + pos) &&
                  strcmp(".cc", file_path.c_str() + pos))) {
    return false;
  }

  // Ignore unittests.
  pos = file_path.length() - 12;
  if (pos >= 12 && !strcmp("_unittest.cc", file_path.c_str() + pos))
    return false;

  base::FilePath converted_file_path =
#if defined(OS_WIN)
      base::FilePath(
          base::FilePath::StringPieceType(base::UTF8ToWide(file_path)));
#else
      base::FilePath(base::FilePath::StringPieceType(file_path));
#endif

  // Check file content.
  std::string file_content;
  if (!base::ReadFileToString(converted_file_path, &file_content)) {
    LOG(ERROR) << "Could not open file: " << file_path;
    return false;
  }

  for (int i = 0; kRelevantKeywords[i]; i++) {
    if (file_content.find(kRelevantKeywords[i]) != std::string::npos)
      return true;
  }

  return false;
}

void TrafficAnnotationFileFilter::GetRelevantFiles(
    const base::FilePath& source_path,
    const std::vector<std::string>& ignore_list,
    std::string directory_name,
    std::vector<std::string>* file_paths) {
  LOG(ERROR) << "GETTING FILES LIST FROM GIT.";
  if (!git_files_.size())
    GetFilesFromGit(source_path);

#if defined(FILE_PATH_USES_WIN_SEPARATORS)
  std::replace(directory_name.begin(), directory_name.end(), L'\\', L'/');
#endif

  size_t name_length = directory_name.length();
  for (const std::string& file_path : git_files_) {
    if (!strncmp(file_path.c_str(), directory_name.c_str(), name_length)) {
      bool ignore = false;
      for (const std::string& ignore_path : ignore_list) {
        if (!strncmp(file_path.c_str(), ignore_path.c_str(),
                     ignore_path.length())) {
          ignore = true;
          break;
        }
      }
      if (!ignore)
        file_paths->push_back(file_path);
    }
  }
  LOG(ERROR) << "GIT DONE WITH " << file_paths->size() << " FILES.";
  // std::string report;
  // for (int i = 0; kRelevantKeywords[i]; i++) {
  //   report += kRelevantKeywords[i];
  //   report += ",";
  // }
  // report += "\n";
  // for (const auto& file_path : *file_paths) {
  //   report += file_path + ",";

  //   // Check file content.
  //   std::string file_content;
  //   if (!base::ReadFileToString(
  //           base::FilePath(base::FilePath::StringPieceType(file_path)),
  //           &file_content)) {
  //     LOG(ERROR) << "Could not open file: " << file_path;
  //     report += "\n";
  //     continue;
  //   }

  //   for (int i = 0; kRelevantKeywords[i]; i++) {
  //     if (file_content.find(kRelevantKeywords[i]) != std::string::npos)
  //       report += "Y,";
  //     else
  //       report += ",";
  //   }
  //   report += "\n";
  // }

  // base::WriteFile(
  //     base::FilePath("/usr/local/google/home/rhalavati/Desktop/filter.txt"),
  //     report.c_str(), report.length());
}

bool GetIncludePathsFromCompileCommands(
    const base::FilePath& build_path,
    const std::vector<std::string>& target_files,
    std::map<base::FilePath, std::vector<base::FilePath>>* include_paths) {
  std::string file_content;
  if (base::ReadFileToString(build_path.Append(base::FilePath::StringPieceType(
                                 "compile_commands.json")),
                             &file_content)) {
    std::vector<std::string> lines = base::SplitString(
        file_content, "\n", base::KEEP_WHITESPACE, base::SPLIT_WANT_ALL);
    for (unsigned i = 1; i < lines.size(); i++) {
      // Get Target File Name
      std::string::size_type pos = lines[i].find("\"file\"");
      if (pos != std::string::npos) {
        std::string file_name =
            lines[i].substr(pos + 9, lines[i].length() - 10 - pos);
        base::FilePath file_path = base::MakeAbsoluteFilePath(
            build_path.Append(base::FilePath::StringPieceType(file_name)));

        // Get Include Paths.
        std::vector<base::FilePath> paths;
        pos = 0;
        while (true) {
          pos = lines[i - 1].find("-I", pos);
          if (pos == std::string::npos)
            break;
          std::string::size_type end_pos = lines[i - 1].find(" ", pos);
          std::string include_path =
              lines[i - 1].substr(pos + 2, end_pos - pos - 2);
          pos = end_pos;
          base::FilePath absolute_path = base::MakeAbsoluteFilePath(
              build_path.Append(base::FilePath::StringPieceType(include_path)));
          if (!base::ContainsValue(paths, absolute_path))
            paths.push_back(absolute_path);
        }
        include_paths->insert(std::make_pair(file_path, paths));
      }
    }
    LOG(INFO) << "compile_commands.json read with " << include_paths->size()
              << " items.";
    return true;
  }

  LOG(ERROR) << "Could not read compile_commands.json";
  return false;
}

bool TrafficAnnotationFileFilter::GetAllRequiredFiles(
    const base::FilePath& source_path,
    const base::FilePath& build_path,
    const std::vector<std::string>& target_files,
    std::vector<std::string>* required_files) {
  // Load Compile Command
  std::map<base::FilePath, std::vector<base::FilePath>> include_paths;
  if (!GetIncludePathsFromCompileCommands(build_path, target_files,
                                          &include_paths))
    return false;

  std::set<base::FilePath> visited;
  std::set<std::string> library;
  std::vector<std::pair<base::FilePath, std::vector<base::FilePath>*>> queue;
  for (auto& file_path : target_files) {
    base::FilePath absolute_path = base::MakeAbsoluteFilePath(
        source_path.Append(base::FilePath::StringPieceType(file_path)));
    if (!base::ContainsKey(include_paths, absolute_path)) {
      LOG(ERROR) << "Could not find " << absolute_path.MaybeAsASCII().c_str()
                 << " in compile_commands.json";
      continue;
    }
    queue.push_back(
        std::make_pair(absolute_path, &include_paths[absolute_path]));
  }

  while (queue.size()) {
    base::FilePath new_path = queue.back().first;
    std::vector<base::FilePath>* include_paths = queue.back().second;
    queue.pop_back();

    visited.insert(new_path);
    base::FilePath current_dir = new_path.DirName();

    // Check file content.
    std::string file_content;
    if (!base::ReadFileToString(new_path, &file_content)) {
      LOG(ERROR) << "Could not open file: " << new_path;
      continue;
    }

    // Iterate all lines in the file.
    for (const std::string line : base::SplitString(
             file_content, "\n", base::KEEP_WHITESPACE, base::SPLIT_WANT_ALL)) {
      // Get included file name.
      if (line.length() > 8 && line.substr(0, 9) == "#include ") {
        std::string included;
        if (line.substr(9, 1) == "<") {
          included = line.substr(10, line.find(">") - 10);
        } else if (line.substr(9, 1) == "\"") {
          included = line.substr(10, line.find("\"", 11) - 10);
        } else {
          LOG(ERROR) << "CANNOT PARSE: " << line;
          continue;
        }

        // Search for the file.
        base::FilePath file_path =
            source_path.Append(base::FilePath::StringPieceType(included));
        if (!base::PathExists(file_path)) {
          // In the directory of parent file?
          file_path =
              current_dir.Append(base::FilePath::StringPieceType(included));
          if (!base::PathExists(file_path)) {
            // In the include files?
            for (const base::FilePath& include_path : *include_paths) {
              file_path = include_path.Append(
                  base::FilePath::StringPieceType(included));
              if (base::PathExists(file_path))
                break;
            }
          }
        }

        if (!base::PathExists(file_path)) {
          // It's probably a library file as it is not found.
          library.insert(included);
        } else {
          file_path = base::MakeAbsoluteFilePath(file_path);
          if (!base::PathExists(file_path)) {
            LOG(ERROR) << "UNEXPECTED NOT FOUND " << file_path.MaybeAsASCII();
          }
          if (!base::ContainsKey(visited, file_path) &&
              !base::ContainsValue(queue,
                                   std::make_pair(file_path, include_paths))) {
            queue.push_back(std::make_pair(file_path, include_paths));
          }
        }
      }
    }
  }

  int source_path_length =
      base::MakeAbsoluteFilePath(source_path).MaybeAsASCII().length();
  for (const base::FilePath& file_path : visited) {
    std::string local_name = file_path.MaybeAsASCII();
    local_name = local_name.substr(source_path_length + 1,
                                   local_name.length() - source_path_length);
    required_files->push_back(local_name);
  }

  std::string report;
  report += "[REQUIRED]\n";
  int i = 0;
  for (const auto& file_path : *required_files) {
    report += base::StringPrintf("%4i %s\n", i, file_path.c_str());
    i++;
  }

  report += "\n\n[LIBRARY]\n";
  i = 0;
  for (const auto& file_path : library) {
    report += base::StringPrintf("%4i %s\n", i, file_path.c_str());
    i++;
  }

  base::WriteFile(
      base::FilePath(
          "/usr/local/google/home/rhalavati/Desktop/requirements.txt"),
      report.c_str(), report.length());

  // COPY
  for (const base::FilePath& file_path : visited) {
    std::string dest_str = file_path.MaybeAsASCII();
    dest_str.find("chromium");
    dest_str.replace(dest_str.find("chromium_clanged"), 16, "CHROMIUM");
    base::FilePath destination =
        base::FilePath(base::FilePath::StringPieceType(dest_str));
    base::CreateDirectory(destination.DirName());
    LOG(INFO) << destination.DirName();
    base::CopyFile(file_path, destination);
  }

  return false;
}