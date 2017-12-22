// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/model/offline_page_model_utils.h"

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/memory/ptr_util.h"
#include "base/strings/utf_string_conversions.h"
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

class OfflinePageModelUtilsTest : public testing::Test {
 public:
  void SetUp() override;

  void PumpLoop() { scoped_task_environment_.RunUntilIdle(); }

  const base::FilePath& target_dir() { return temp_dir_.GetPath(); }

 private:
  base::test::ScopedTaskEnvironment scoped_task_environment_;
  base::ScopedTempDir temp_dir_;
};

void OfflinePageModelUtilsTest::SetUp() {
  ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
}

TEST_F(OfflinePageModelUtilsTest, ToNamespaceEnum) {
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

TEST_F(OfflinePageModelUtilsTest, GenerateUniqueFileName) {
  // The MockCallback is used as the callback passed into file name generation.
  // The Run() method is expected to be called with the expected result and in
  // order to enable the counter of suffix to increase, every time the callback
  // is called, we create a placeholder file in the directory using
  // WillOnce(Invoke(WriteFileAsPlaceHolder)), WriteFileAsPlaceHolder will take
  // the returned value in the callback as the input argument;
  base::MockCallback<model_utils::GenerateUniqueFileNameCallback> callback;

  GURL url_1("http://news.google.com/page1");
  base::string16 title_1 = base::UTF8ToUTF16("Google News Page");

  base::FilePath expected_1(FILE_PATH_LITERAL("Google News Page.mhtml"));
  EXPECT_CALL(callback, Run(Eq(target_dir().Append(expected_1))))
      .WillOnce(Invoke(WriteFileAsPlaceHolder));

  model_utils::GenerateUniqueFileName(target_dir(), url_1, title_1,
                                      callback.Get());
  PumpLoop();

  // Adding same file again should introduce suffix "(1)".
  base::FilePath expected_1_dup1(
      FILE_PATH_LITERAL("Google News Page (1).mhtml"));
  EXPECT_CALL(callback, Run(Eq(target_dir().Append(expected_1_dup1))))
      .WillOnce(Invoke(WriteFileAsPlaceHolder));

  model_utils::GenerateUniqueFileName(target_dir(), url_1, title_1,
                                      callback.Get());
  PumpLoop();

  // Adding same file again should introduce suffix "(2)".
  base::FilePath expected_1_dup2(
      FILE_PATH_LITERAL("Google News Page (2).mhtml"));
  EXPECT_CALL(callback, Run(Eq(target_dir().Append(expected_1_dup2))))
      .WillOnce(Invoke(WriteFileAsPlaceHolder));

  model_utils::GenerateUniqueFileName(target_dir(), url_1, title_1,
                                      callback.Get());
  PumpLoop();

  // Try another url and title.
  GURL url_2("https://en.m.wikipedia.org/Sample_page_about_stuff");
  base::string16 title_2 = base::UTF8ToUTF16("Some Wiki Page - Wikipedia");

  base::FilePath expected_2(
      FILE_PATH_LITERAL("Some Wiki Page - Wikipedia.mhtml"));
  EXPECT_CALL(callback, Run(Eq(target_dir().Append(expected_2))))
      .WillOnce(Invoke(WriteFileAsPlaceHolder));

  model_utils::GenerateUniqueFileName(target_dir(), url_2, title_2,
                                      callback.Get());
  PumpLoop();
}

TEST_F(OfflinePageModelUtilsTest, GenerateUniqueFileName_LongTitle) {
  base::MockCallback<model_utils::GenerateUniqueFileNameCallback> callback;

  // Try a long title.
  GURL url("https://www.google.com/search");
  base::string16 title = base::UTF8ToUTF16(
      "A really really really really really long long^ - LONG LONG TITLE");
  base::FilePath expected(
      FILE_PATH_LITERAL("A really really really really really long long^ - "
                        "LONG LONG TITLE.mhtml"));
  EXPECT_CALL(callback, Run(Eq(target_dir().Append(expected))))
      .WillOnce(Invoke(WriteFileAsPlaceHolder));

  model_utils::GenerateUniqueFileName(target_dir(), url, title, callback.Get());
  PumpLoop();

  // Try the long title again, to see suffix.
  base::FilePath expected_dup(
      FILE_PATH_LITERAL("A really really really really really long long^ - "
                        "LONG LONG TITLE (1).mhtml"));
  EXPECT_CALL(callback, Run(Eq(target_dir().Append(expected_dup))))
      .WillOnce(Invoke(WriteFileAsPlaceHolder));

  model_utils::GenerateUniqueFileName(target_dir(), url, title, callback.Get());
  PumpLoop();
}

TEST_F(OfflinePageModelUtilsTest, GenerateUniqueFileName_FillingHoles) {
  base::MockCallback<model_utils::GenerateUniqueFileNameCallback> callback;

  GURL url("http://news.google.com/page1");
  base::string16 title = base::UTF8ToUTF16("Google News Page");
  base::FilePath expected(FILE_PATH_LITERAL("Google News Page.mhtml"));
  EXPECT_CALL(callback, Run(Eq(target_dir().Append(expected))))
      .WillOnce(Invoke(WriteFileAsPlaceHolder));

  model_utils::GenerateUniqueFileName(target_dir(), url, title, callback.Get());
  PumpLoop();

  // Adding same file again should introduce suffix "(1)".
  base::FilePath expected_dup1(FILE_PATH_LITERAL("Google News Page (1).mhtml"));
  EXPECT_CALL(callback, Run(Eq(target_dir().Append(expected_dup1))))
      .WillOnce(Invoke(WriteFileAsPlaceHolder));

  model_utils::GenerateUniqueFileName(target_dir(), url, title, callback.Get());
  PumpLoop();

  // The last suffix of the file names is (99).
  // For the file names returned via the callback, we will save them and:
  //  1. Create a placeholder file on the disk, to simulate archive creation.
  //  2. Confirm the path returned is not empty, since it's still within the
  //     limit of unique file name.
  //  3. Confirm the suffix counter was increased successfully.
  base::FilePath file_path;
  EXPECT_CALL(callback, Run(A<base::FilePath>()))
      .WillRepeatedly(SaveArg<0>(&file_path));
  for (int i = 2; i < 100; i++) {
    model_utils::GenerateUniqueFileName(target_dir(), url, title,
                                        callback.Get());
    PumpLoop();
    EXPECT_EQ(0, base::WriteFile(file_path, NULL, 0));
    EXPECT_FALSE(file_path.empty());
    EXPECT_TRUE(file_path.value().find(std::to_string(i)) != std::string::npos);
  }

  // Delete the name with (42).
  base::FilePath file_42(
      target_dir().Append(FILE_PATH_LITERAL("Google News Page (42).mhtml")));
  ASSERT_TRUE(base::DeleteFile(file_42, false));

  // Try generating file name one more time and it will return a file path
  // ending with (42).
  model_utils::GenerateUniqueFileName(target_dir(), url, title, callback.Get());
  PumpLoop();
  EXPECT_TRUE(file_path.value().find("42") != std::string::npos);
}

TEST_F(OfflinePageModelUtilsTest,
       GenerateUniqueFileName_NameWithIllegalCharacters) {
  base::MockCallback<model_utils::GenerateUniqueFileNameCallback> callback;

  GURL url("http://news.google.com/page1");
  base::string16 title = base::UTF8ToUTF16("Google/\\\?\%*:|\"<>. News Page");
  base::FilePath expected(
      FILE_PATH_LITERAL("Google___%______. News Page.mhtml"));
  EXPECT_CALL(callback, Run(Eq(target_dir().Append(expected))))
      .WillOnce(Invoke(WriteFileAsPlaceHolder));

  model_utils::GenerateUniqueFileName(target_dir(), url, title, callback.Get());
  PumpLoop();
}

TEST_F(OfflinePageModelUtilsTest, GenerateUniqueFileName_NameWithUnicode) {
  base::MockCallback<model_utils::GenerateUniqueFileNameCallback> callback;

  GURL url("http://news.google.com/page1");
  base::string16 title =
      base::UTF8ToUTF16("Google News Page ßiÂÃÄÅÆÇÈÉ ïðñòóôõö÷øùúûü");
  base::FilePath expected(
      FILE_PATH_LITERAL("Google News Page ßiÂÃÄÅÆÇÈÉ ïðñòóôõö÷øùúûü.mhtml"));
  EXPECT_CALL(callback, Run(Eq(target_dir().Append(expected))))
      .WillOnce(Invoke(WriteFileAsPlaceHolder));

  model_utils::GenerateUniqueFileName(target_dir(), url, title, callback.Get());
  PumpLoop();
}

TEST_F(OfflinePageModelUtilsTest, GenerateUniqueFileName_EmptyTitle) {
  base::MockCallback<model_utils::GenerateUniqueFileNameCallback> callback;

  GURL url("http://news.google.com/page1");
  base::string16 title = base::UTF8ToUTF16("");
  base::FilePath expected(FILE_PATH_LITERAL("NO TITLE.mhtml"));
  EXPECT_CALL(callback, Run(Eq(target_dir().Append(expected))))
      .WillOnce(Invoke(WriteFileAsPlaceHolder));

  model_utils::GenerateUniqueFileName(target_dir(), url, title, callback.Get());
  PumpLoop();
}

}  // namespace offline_pages
