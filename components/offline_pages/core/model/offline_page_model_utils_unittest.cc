// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/model/offline_page_model_utils.h"

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/memory/ptr_util.h"
#include "base/test/mock_callback.h"
#include "base/test/scoped_task_environment.h"
#include "base/test/test_simple_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "components/offline_pages/core/client_id.h"
#include "components/offline_pages/core/client_namespace_constants.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

using testing::A;
using testing::Eq;
using testing::Invoke;
using testing::SaveArg;

namespace offline_pages {

namespace {

// Used to create a placeholder file for |file_path| so that it's possible to
// generate file names with suffixes.
void WriteFileAsPlaceHolder(const base::FilePath& file_path) {
  EXPECT_EQ(0, base::WriteFile(file_path, NULL, 0));
}

}  // namespace

TEST(OfflinePageModelUtilsTest, ToNamespaceEnum) {
  EXPECT_EQ(model_utils::ToNamespaceEnum(kDefaultNamespace),
            OfflinePagesNamespaceEnumeration::DEFAULT);
  EXPECT_EQ(model_utils::ToNamespaceEnum(kBookmarkNamespace),
            OfflinePagesNamespaceEnumeration::BOOKMARK);
  EXPECT_EQ(model_utils::ToNamespaceEnum(kLastNNamespace),
            OfflinePagesNamespaceEnumeration::LAST_N);
  EXPECT_EQ(model_utils::ToNamespaceEnum(kAsyncNamespace),
            OfflinePagesNamespaceEnumeration::ASYNC_LOADING);
  EXPECT_EQ(model_utils::ToNamespaceEnum(kCCTNamespace),
            OfflinePagesNamespaceEnumeration::CUSTOM_TABS);
  EXPECT_EQ(model_utils::ToNamespaceEnum(kDownloadNamespace),
            OfflinePagesNamespaceEnumeration::DOWNLOAD);
  EXPECT_EQ(model_utils::ToNamespaceEnum(kNTPSuggestionsNamespace),
            OfflinePagesNamespaceEnumeration::NTP_SUGGESTION);
  EXPECT_EQ(model_utils::ToNamespaceEnum(kSuggestedArticlesNamespace),
            OfflinePagesNamespaceEnumeration::SUGGESTED_ARTICLES);
  EXPECT_EQ(model_utils::ToNamespaceEnum(kBrowserActionsNamespace),
            OfflinePagesNamespaceEnumeration::BROWSER_ACTIONS);
}

TEST(OfflinePageModelUtilsTest, AddHistogramSuffix) {
  EXPECT_EQ(model_utils::AddHistogramSuffix(ClientId(kDefaultNamespace, "id"),
                                            "OfflinePages.HistogramName"),
            "OfflinePages.HistogramName.default");
  EXPECT_EQ(model_utils::AddHistogramSuffix(ClientId(kLastNNamespace, "id"),
                                            "OfflinePages.HistogramName"),
            "OfflinePages.HistogramName.last_n");
  EXPECT_EQ(model_utils::AddHistogramSuffix(ClientId(kDownloadNamespace, "id"),
                                            "OfflinePages.HistogramName"),
            "OfflinePages.HistogramName.download");
}

TEST(OfflinePageModelUtilsTest, GenerateUniqueFileName_Success) {
  // Setting the environment so that there's a thread task runner for testing.
  base::test::ScopedTaskEnvironment scoped_task_environment;

  // Creating a ScopedTempDir for simulating the archives directory.
  base::ScopedTempDir temp_dir;
  ASSERT_TRUE(temp_dir.CreateUniqueTempDir());
  base::FilePath target_dir = temp_dir.GetPath();
  // The MockCallback is used as the callback passed into file name generation.
  // The Run() method is expected to be called with the expected result and in
  // order to enable the counter of suffix to increase, every time the callback
  // is called, we create a placeholder file in the directory using
  // WillOnce(Invoke(WriteFileAsPlaceHolder)), WriteFileAsPlaceHolder will take
  // the returned value in the callback as the input argument;
  base::MockCallback<model_utils::GenerateUniqueFileNameCallback> callback;

  GURL url_1("http://news.google.com/page1");
  std::string title_1("Google News Page");

  base::FilePath expected_1(
      FILE_PATH_LITERAL("news.google.com-Google_News_Page.mhtml"));
  EXPECT_CALL(callback, Run(Eq(target_dir.Append(expected_1))))
      .WillOnce(Invoke(WriteFileAsPlaceHolder));

  model_utils::GenerateUniqueFileName(target_dir, url_1, title_1,
                                      callback.Get());
  scoped_task_environment.RunUntilIdle();

  // Adding same file again should introduce suffix "(1)".
  base::FilePath expected_1_dup1(
      FILE_PATH_LITERAL("news.google.com-Google_News_Page (1).mhtml"));
  EXPECT_CALL(callback, Run(Eq(target_dir.Append(expected_1_dup1))))
      .WillOnce(Invoke(WriteFileAsPlaceHolder));

  model_utils::GenerateUniqueFileName(target_dir, url_1, title_1,
                                      callback.Get());
  scoped_task_environment.RunUntilIdle();

  // Adding same file again should introduce suffix "(2)".
  base::FilePath expected_1_dup2(
      FILE_PATH_LITERAL("news.google.com-Google_News_Page (2).mhtml"));
  EXPECT_CALL(callback, Run(Eq(target_dir.Append(expected_1_dup2))))
      .WillOnce(Invoke(WriteFileAsPlaceHolder));

  model_utils::GenerateUniqueFileName(target_dir, url_1, title_1,
                                      callback.Get());
  scoped_task_environment.RunUntilIdle();

  // Try another url and title.
  GURL url_2("https://en.m.wikipedia.org/Sample_page_about_stuff");
  std::string title_2("Some Wiki Page");

  base::FilePath expected_2(
      FILE_PATH_LITERAL("en.m.wikipedia.org-Some_Wiki_Page.mhtml"));
  EXPECT_CALL(callback, Run(Eq(target_dir.Append(expected_2))))
      .WillOnce(Invoke(WriteFileAsPlaceHolder));

  model_utils::GenerateUniqueFileName(target_dir, url_2, title_2,
                                      callback.Get());
  scoped_task_environment.RunUntilIdle();

  // Try a long title.
  GURL url_3("https://www.google.com/search");
  std::string title_3 =
      "A really really really really long long^ - TRUNCATE THIS PART ";
  std::string expected_title_3_part =
      "A_really_really_really_really_long_long^";
  base::FilePath expected_3(
      FILE_PATH_LITERAL("www.google.com-" + expected_title_3_part + ".mhtml"));
  EXPECT_CALL(callback, Run(Eq(target_dir.Append(expected_3))))
      .WillOnce(Invoke(WriteFileAsPlaceHolder));

  model_utils::GenerateUniqueFileName(target_dir, url_3, title_3,
                                      callback.Get());
  scoped_task_environment.RunUntilIdle();

  // Try the long title again, to see suffix.
  base::FilePath expected_3_dup(FILE_PATH_LITERAL(
      "www.google.com-" + expected_title_3_part + " (1).mhtml"));
  EXPECT_CALL(callback, Run(Eq(target_dir.Append(expected_3_dup))))
      .WillOnce(Invoke(WriteFileAsPlaceHolder));

  model_utils::GenerateUniqueFileName(target_dir, url_3, title_3,
                                      callback.Get());
  scoped_task_environment.RunUntilIdle();
}

TEST(OfflinePageModelUtilsTest, GenerateUniqueFileName_ExceedsLimit) {
  base::test::ScopedTaskEnvironment scoped_task_environment;

  base::ScopedTempDir temp_dir;
  ASSERT_TRUE(temp_dir.CreateUniqueTempDir());
  base::FilePath target_dir = temp_dir.GetPath();
  base::MockCallback<model_utils::GenerateUniqueFileNameCallback> callback;

  GURL url_1("http://news.google.com/page1");
  std::string title_1("Google News Page");
  base::FilePath expected_1(
      FILE_PATH_LITERAL("news.google.com-Google_News_Page.mhtml"));
  EXPECT_CALL(callback, Run(Eq(target_dir.Append(expected_1))))
      .WillOnce(Invoke(WriteFileAsPlaceHolder));

  model_utils::GenerateUniqueFileName(target_dir, url_1, title_1,
                                      callback.Get());
  scoped_task_environment.RunUntilIdle();

  // Adding same file again should introduce suffix "(1)".
  base::FilePath expected_1_dup1(
      FILE_PATH_LITERAL("news.google.com-Google_News_Page (1).mhtml"));
  EXPECT_CALL(callback, Run(Eq(target_dir.Append(expected_1_dup1))))
      .WillOnce(Invoke(WriteFileAsPlaceHolder));

  model_utils::GenerateUniqueFileName(target_dir, url_1, title_1,
                                      callback.Get());
  scoped_task_environment.RunUntilIdle();

  // The last available suffix of the file names is (99).
  // For the file names returned via the callback, we will save them and:
  //  1. Create a placeholder file on the disk, to simulate archive creation.
  //  2. Confirm the path returned is not empty, since it's still within the
  //     limit of unique file name.
  //  3. Confirm the suffix counter was increased successfully.
  base::FilePath file_path;
  EXPECT_CALL(callback, Run(A<base::FilePath>()))
      .WillRepeatedly(SaveArg<0>(&file_path));
  for (int i = 2; i < 100; i++) {
    model_utils::GenerateUniqueFileName(target_dir, url_1, title_1,
                                        callback.Get());
    scoped_task_environment.RunUntilIdle();
    EXPECT_EQ(0, base::WriteFile(file_path, NULL, 0));
    EXPECT_FALSE(file_path.empty());
    EXPECT_TRUE(file_path.value().find(std::to_string(i)) != std::string::npos);
  }

  // Try generating file name one more time and it will return an empty path
  // since it's over the limit of 100 files with same names (0-99).
  model_utils::GenerateUniqueFileName(target_dir, url_1, title_1,
                                      callback.Get());
  scoped_task_environment.RunUntilIdle();
  EXPECT_TRUE(file_path.empty());
}

}  // namespace offline_pages
