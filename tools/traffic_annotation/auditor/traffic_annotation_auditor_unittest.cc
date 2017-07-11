// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "tools/traffic_annotation/auditor/traffic_annotation_auditor.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "tools/traffic_annotation/auditor/traffic_annotation_file_filter.h"

namespace {

#define TEST_HASH_CODE(X)                                  \
  EXPECT_EQ(TrafficAnnotationAuditor::ComputeHashValue(X), \
            net::DefineNetworkTrafficAnnotation(X, "").unique_id_hash_code)

const char* kIrrelevantFiles[] = {
    "tools/traffic_annotation/auditor/tests/git_list.txt",
    "tools/traffic_annotation/auditor/tests/irrelevent_file_content.cc",
    "tools/traffic_annotation/auditor/tests/irrelevent_file_content.mm",
    "tools/traffic_annotation/auditor/tests/irrelevent_file_name.txt", nullptr};

const char* kRelevantFiles[] = {
    "tools/traffic_annotation/auditor/tests/relevent_file_name_and_content.cc",
    "tools/traffic_annotation/auditor/tests/relevent_file_name_and_content.mm",
    nullptr};
}

using namespace testing;

class TrafficAnnotationAuditorTest : public ::testing::Test {
 public:
  void SetUp() override {
    if (!base::GetCurrentDirectory(&source_path_)) {
      LOG(ERROR) << "Could not get current directory to find source path.";
      return;
    }
    git_list_mock_file_ = source_path_.Append(FILE_PATH_LITERAL("tools"))
                              .Append(FILE_PATH_LITERAL("traffic_annotation"))
                              .Append(FILE_PATH_LITERAL("auditor"))
                              .Append(FILE_PATH_LITERAL("tests"))
                              .Append(FILE_PATH_LITERAL("git_list.txt"));
  }

 protected:
  base::FilePath source_path_;
  base::FilePath build_path_;  // Currently stays empty. Will be set if access
                               // to a compiled build directory would be
                               // granted.
  base::FilePath git_list_mock_file_;
};

// Tests if the two hash computation functions have the same result.
TEST_F(TrafficAnnotationAuditorTest, HashFunctionCheck) {
  TEST_HASH_CODE("test");
  TEST_HASH_CODE("unique_id");
  TEST_HASH_CODE("123_id");
  TEST_HASH_CODE("ID123");
  TEST_HASH_CODE(
      "a_unique_looooooooooooooooooooooooooooooooooooooooooooooooooooooong_id");
}

// Tests if TrafficAnnotationFileFilter::GetFilesFromGit function returns
// correct files given a mock git list file. It also inherently checks
// TrafficAnnotationFileFilter::IsFileRelevant.
TEST_F(TrafficAnnotationAuditorTest, GetFilesFromGit) {
  TrafficAnnotationFileFilter filter;
  filter.GetFilesFromGit(base::FilePath(), git_list_mock_file_);

  const std::vector<std::string> git_files = filter.get_git_files_for_test();

  unsigned int index = 0;
  while (kRelevantFiles[index]) {
    EXPECT_TRUE(std::find(git_files.begin(), git_files.end(),
                          kRelevantFiles[index]) != git_files.end());
    index++;
  }
  EXPECT_TRUE(git_files.size() == index);

  index = 0;
  while (kIrrelevantFiles[index]) {
    EXPECT_TRUE(std::find(git_files.begin(), git_files.end(),
                          kIrrelevantFiles[index]) == git_files.end());
    index++;
  }
}

// Tests if TrafficAnnotationFileFilter::GetRelevantFiles gives the correct list
// of files, given a mock git list file.
TEST_F(TrafficAnnotationAuditorTest, ReleventFilesReceived) {
  TrafficAnnotationFileFilter filter;
  filter.GetFilesFromGit(base::FilePath(), git_list_mock_file_);

  unsigned int git_files_count = filter.get_git_files_for_test().size();

  std::vector<std::string> ignore_list;
  std::vector<std::string> file_paths;

  // Check if all files are returned with no ignore list and directory.
  filter.GetRelevantFiles(base::FilePath(), ignore_list, "", &file_paths);
  EXPECT_TRUE(file_paths.size() == git_files_count);

  // Check if a file is ignored if it is added to ignore list.
  ignore_list.push_back(file_paths[0]);
  file_paths.clear();
  filter.GetRelevantFiles(base::FilePath(), ignore_list, "", &file_paths);
  EXPECT_TRUE(file_paths.size() == git_files_count - 1);

  // Check if files are filtered based on given directory.
  ignore_list.clear();
  file_paths.clear();
  filter.GetRelevantFiles(base::FilePath(), ignore_list,
                          "tools/traffic_annotation", &file_paths);
  EXPECT_TRUE(file_paths.size() == git_files_count);
  file_paths.clear();
  filter.GetRelevantFiles(base::FilePath(), ignore_list, "content",
                          &file_paths);
  EXPECT_TRUE(file_paths.size() == 0);
}

// Tests if TrafficAnnotationFileFilter::IsWhitelisted works as expected.
// Inherently checks if TrafficAnnotationFileFilter::LoadWhiteList works and
// AuditorException rules are correctly deserialized.
TEST_F(TrafficAnnotationAuditorTest, IsWhitelisted) {
  TrafficAnnotationAuditor auditor(source_path_, build_path_);

  for (unsigned int i = 0;
       i < static_cast<unsigned int>(
               AuditorException::ExceptionType::EXCEPTION_TYPE_LAST);
       i++) {
    AuditorException::ExceptionType type =
        static_cast<AuditorException::ExceptionType>(i);
    // Anything in /tools directory is whitelisted for all types.
    EXPECT_TRUE(auditor.IsWhitelisted("tools/something.cc", type));
    EXPECT_TRUE(auditor.IsWhitelisted("tools/somewhere/something.mm", type));

    // Anything in a general folder is not whitelisted for any type
    EXPECT_FALSE(auditor.IsWhitelisted("something.cc", type));
    EXPECT_FALSE(auditor.IsWhitelisted("content/something.mm", type));
  }

  // Files defining missing annotation functions in net/ are exceptions of
  // 'missing' type.
  EXPECT_TRUE(auditor.IsWhitelisted("net/url_request/url_fetcher.cc",
                                    AuditorException::ExceptionType::MISSING));
  EXPECT_TRUE(auditor.IsWhitelisted("net/url_request/url_request_context.cc",
                                    AuditorException::ExceptionType::MISSING));
}

// Tests if annotation instances are corrrectly deserialized.
TEST_F(TrafficAnnotationAuditorTest, AnnotationDeserilization) {
  // TODO(rhalavati): In next patch set.
}

// Tests if call instances are corrrectly deserialized.
TEST_F(TrafficAnnotationAuditorTest, CallDeserilization) {
  // TODO(rhalavati): In next patch set.
}

// Tests if TrafficAnnotationFileFilter::CheckDuplicateHashes works as expected.
TEST_F(TrafficAnnotationAuditorTest, CheckDuplicateHashes) {
  // TODO(rhalavati): In next patch set.
}

// Tests if TrafficAnnotationFileFilter::RunClangTool works as expected.
TEST_F(TrafficAnnotationAuditorTest, RunClangTool) {
  // TODO(rhalavati): Pending negotiation with infra team.
}

// Tests if TrafficAnnotationFileFilter::ParseClangToolRawOutput works as
// expected.
TEST_F(TrafficAnnotationAuditorTest, ParseClangToolRawOutput) {
  // TODO(rhalavati): Pending negotiation with infra team.
}

// Tests if TrafficAnnotationAuditor::RunClangTool has similar outputs when
// full_run flag has ture and false.
// WARNING: This test takes over 40 minutes, so should be flagged to be run in
// limited cases.
TEST_F(TrafficAnnotationAuditorTest, RunClangToolHeuristicConsistency) {
  // TODO(rhalavati): Pending negotiation with infra team.
}