// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/offline_pages/offline_page_mhtml_archiver.h"

#include <stdint.h>

#include <memory>

#include "base/bind.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/run_loop.h"
#include "base/strings/string16.h"
#include "base/strings/utf_string_conversions.h"
#include "base/test/test_simple_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace offline_pages {

namespace {

const char kTestURL[] = "http://example.com/";
const base::FilePath::CharType kTestFilePath[] = FILE_PATH_LITERAL(
    "/archive_dir/offline_page.mhtml");
const int64_t kTestFileSize = 123456LL;
const base::string16 kTestTitle = base::UTF8ToUTF16("a title");

class TestMHTMLArchiver : public OfflinePageMHTMLArchiver {
 public:
  enum class TestScenario {
    SUCCESS,
    NOT_ABLE_TO_ARCHIVE,
    WEB_CONTENTS_MISSING,
    CONNECTION_SECURITY_ERROR,
    ERROR_PAGE,
    INTERSTITIAL_PAGE,
  };

  TestMHTMLArchiver(const GURL& url, const TestScenario test_scenario);
  ~TestMHTMLArchiver() override;

 private:
  void GenerateMHTML(const base::FilePath& archives_dir,
                     const CreateArchiveParams& create_archive_params) override;
  bool HasConnectionSecurityError() override;
  content::PageType GetPageType() override;

  const GURL url_;
  const TestScenario test_scenario_;

  DISALLOW_COPY_AND_ASSIGN(TestMHTMLArchiver);
};

TestMHTMLArchiver::TestMHTMLArchiver(const GURL& url,
                                     const TestScenario test_scenario)
    : url_(url),
      test_scenario_(test_scenario) {
}

TestMHTMLArchiver::~TestMHTMLArchiver() {
}

void TestMHTMLArchiver::GenerateMHTML(
    const base::FilePath& archives_dir,
    const CreateArchiveParams& create_archive_params) {
  if (test_scenario_ == TestScenario::WEB_CONTENTS_MISSING) {
    ReportFailure(ArchiverResult::ERROR_CONTENT_UNAVAILABLE);
    return;
  }

  if (test_scenario_ == TestScenario::NOT_ABLE_TO_ARCHIVE) {
    ReportFailure(ArchiverResult::ERROR_ARCHIVE_CREATION_FAILED);
    return;
  }

  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE,
      base::Bind(&TestMHTMLArchiver::OnGenerateMHTMLDone,
                 base::Unretained(this), url_, base::FilePath(kTestFilePath),
                 kTestTitle, kTestFileSize));
}

bool TestMHTMLArchiver::HasConnectionSecurityError() {
  return test_scenario_ == TestScenario::CONNECTION_SECURITY_ERROR;
}

content::PageType TestMHTMLArchiver::GetPageType() {
  if (test_scenario_ == TestScenario::ERROR_PAGE)
    return content::PageType::PAGE_TYPE_ERROR;
  if (test_scenario_ == TestScenario::INTERSTITIAL_PAGE)
    return content::PageType::PAGE_TYPE_INTERSTITIAL;
  return content::PageType::PAGE_TYPE_NORMAL;
}

}  // namespace

class OfflinePageMHTMLArchiverTest : public testing::Test {
 public:
  OfflinePageMHTMLArchiverTest();
  ~OfflinePageMHTMLArchiverTest() override;

  // Creates an archiver for testing scenario and uses it to create an archive.
  std::unique_ptr<TestMHTMLArchiver> CreateArchive(
      const GURL& url,
      TestMHTMLArchiver::TestScenario scenario);

  // Test tooling methods.
  void PumpLoop();

  base::FilePath GetTestFilePath() const {
    return base::FilePath(kTestFilePath);
  }

  const OfflinePageArchiver* last_archiver() const { return last_archiver_; }
  OfflinePageArchiver::ArchiverResult last_result() const {
    return last_result_;
  }
  const base::FilePath& last_file_path() const { return last_file_path_; }
  int64_t last_file_size() const { return last_file_size_; }
  const base::ScopedTempDir& temp_dir() { return temp_dir_; }

  const OfflinePageArchiver::CreateArchiveCallback callback() {
    return base::Bind(&OfflinePageMHTMLArchiverTest::OnCreateArchiveDone,
                      base::Unretained(this));
  }

 private:
  void OnCreateArchiveDone(OfflinePageArchiver* archiver,
                           OfflinePageArchiver::ArchiverResult result,
                           const GURL& url,
                           const base::FilePath& file_path,
                           const base::string16& title,
                           int64_t file_size);

  OfflinePageArchiver* last_archiver_;
  OfflinePageArchiver::ArchiverResult last_result_;
  GURL last_url_;
  base::FilePath last_file_path_;
  int64_t last_file_size_;
  base::ScopedTempDir temp_dir_;

  scoped_refptr<base::TestSimpleTaskRunner> task_runner_;
  base::ThreadTaskRunnerHandle task_runner_handle_;

  DISALLOW_COPY_AND_ASSIGN(OfflinePageMHTMLArchiverTest);
};

OfflinePageMHTMLArchiverTest::OfflinePageMHTMLArchiverTest()
    : last_archiver_(nullptr),
      last_result_(OfflinePageArchiver::ArchiverResult::
                       ERROR_ARCHIVE_CREATION_FAILED),
      last_file_size_(0L),
      task_runner_(new base::TestSimpleTaskRunner),
      task_runner_handle_(task_runner_) {
  EXPECT_TRUE(temp_dir_.CreateUniqueTempDir());
}

OfflinePageMHTMLArchiverTest::~OfflinePageMHTMLArchiverTest() {
}

std::unique_ptr<TestMHTMLArchiver> OfflinePageMHTMLArchiverTest::CreateArchive(
    const GURL& url,
    TestMHTMLArchiver::TestScenario scenario) {
  std::unique_ptr<TestMHTMLArchiver> archiver(
      new TestMHTMLArchiver(url, scenario));
  archiver->CreateArchive(GetTestFilePath(),
                          OfflinePageArchiver::CreateArchiveParams(),
                          callback());
  PumpLoop();
  return archiver;
}

void OfflinePageMHTMLArchiverTest::OnCreateArchiveDone(
    OfflinePageArchiver* archiver,
    OfflinePageArchiver::ArchiverResult result,
    const GURL& url,
    const base::FilePath& file_path,
    const base::string16& title,
    int64_t file_size) {
  last_url_ = url;
  last_archiver_ = archiver;
  last_result_ = result;
  last_file_path_ = file_path;
  last_file_size_ = file_size;
}

void OfflinePageMHTMLArchiverTest::PumpLoop() {
  task_runner_->RunUntilIdle();
}

// Tests that creation of an archiver fails when web contents is missing.
TEST_F(OfflinePageMHTMLArchiverTest, WebContentsMissing) {
  GURL page_url = GURL(kTestURL);
  std::unique_ptr<TestMHTMLArchiver> archiver(CreateArchive(
      page_url, TestMHTMLArchiver::TestScenario::WEB_CONTENTS_MISSING));

  EXPECT_EQ(archiver.get(), last_archiver());
  EXPECT_EQ(OfflinePageArchiver::ArchiverResult::ERROR_CONTENT_UNAVAILABLE,
            last_result());
  EXPECT_EQ(base::FilePath(), last_file_path());
}

// Tests for archiver failing save an archive.
TEST_F(OfflinePageMHTMLArchiverTest, NotAbleToGenerateArchive) {
  GURL page_url = GURL(kTestURL);
  std::unique_ptr<TestMHTMLArchiver> archiver(CreateArchive(
      page_url, TestMHTMLArchiver::TestScenario::NOT_ABLE_TO_ARCHIVE));

  EXPECT_EQ(archiver.get(), last_archiver());
  EXPECT_EQ(OfflinePageArchiver::ArchiverResult::ERROR_ARCHIVE_CREATION_FAILED,
            last_result());
  EXPECT_EQ(base::FilePath(), last_file_path());
  EXPECT_EQ(0LL, last_file_size());
}

// Tests for archiver handling of non-secure connection.
TEST_F(OfflinePageMHTMLArchiverTest, ConnectionNotSecure) {
  GURL page_url = GURL(kTestURL);
  std::unique_ptr<TestMHTMLArchiver> archiver(CreateArchive(
      page_url, TestMHTMLArchiver::TestScenario::CONNECTION_SECURITY_ERROR));

  EXPECT_EQ(archiver.get(), last_archiver());
  EXPECT_EQ(OfflinePageArchiver::ArchiverResult::ERROR_SECURITY_CERTIFICATE,
            last_result());
  EXPECT_EQ(base::FilePath(), last_file_path());
  EXPECT_EQ(0LL, last_file_size());
}

// Tests for archiver handling of an error page.
TEST_F(OfflinePageMHTMLArchiverTest, PageError) {
  GURL page_url = GURL(kTestURL);
  std::unique_ptr<TestMHTMLArchiver> archiver(
      CreateArchive(page_url, TestMHTMLArchiver::TestScenario::ERROR_PAGE));

  EXPECT_EQ(archiver.get(), last_archiver());
  EXPECT_EQ(OfflinePageArchiver::ArchiverResult::ERROR_ERROR_PAGE,
            last_result());
  EXPECT_EQ(base::FilePath(), last_file_path());
  EXPECT_EQ(0LL, last_file_size());
}

// Tests for archiver handling of an interstitial page.
TEST_F(OfflinePageMHTMLArchiverTest, InterstitialPage) {
  GURL page_url = GURL(kTestURL);
  std::unique_ptr<TestMHTMLArchiver> archiver(CreateArchive(
      page_url, TestMHTMLArchiver::TestScenario::INTERSTITIAL_PAGE));
  EXPECT_EQ(archiver.get(), last_archiver());
  EXPECT_EQ(OfflinePageArchiver::ArchiverResult::ERROR_INTERSTITIAL_PAGE,
            last_result());
  EXPECT_EQ(base::FilePath(), last_file_path());
  EXPECT_EQ(0LL, last_file_size());
}

// Tests for successful creation of the offline page archive.
TEST_F(OfflinePageMHTMLArchiverTest, SuccessfullyCreateOfflineArchive) {
  GURL page_url = GURL(kTestURL);
  std::unique_ptr<TestMHTMLArchiver> archiver(
      CreateArchive(page_url, TestMHTMLArchiver::TestScenario::SUCCESS));
  PumpLoop();

  EXPECT_EQ(archiver.get(), last_archiver());
  EXPECT_EQ(OfflinePageArchiver::ArchiverResult::SUCCESSFULLY_CREATED,
            last_result());
  EXPECT_EQ(GetTestFilePath(), last_file_path());
  EXPECT_EQ(kTestFileSize, last_file_size());
}

// Test for the generation of file names.
TEST_F(OfflinePageMHTMLArchiverTest, GenerateFileName) {
  base::FilePath archives_dir = temp_dir().GetPath();

  GURL url_1("http://news.google.com/page1");
  std::string title_1("Google News Page");
  base::FilePath expected_1(
      FILE_PATH_LITERAL("news.google.com-Google_News_Page.mhtml"));
  base::FilePath actual_1(
      OfflinePageMHTMLArchiver::GenerateFileName(archives_dir, url_1, title_1));
  EXPECT_EQ(0, base::WriteFile(actual_1, NULL, 0));
  EXPECT_EQ(archives_dir.Append(expected_1), actual_1);

  // Adding same file again should introduce suffices "(1)".
  base::FilePath expected_1_dup1(
      FILE_PATH_LITERAL("news.google.com-Google_News_Page (1).mhtml"));
  base::FilePath actual_1_dup1(
      OfflinePageMHTMLArchiver::GenerateFileName(archives_dir, url_1, title_1));
  EXPECT_EQ(0, base::WriteFile(actual_1_dup1, NULL, 0));
  EXPECT_EQ(archives_dir.Append(expected_1_dup1), actual_1_dup1);

  // Adding same file again should introduce suffices "(2)".
  base::FilePath expected_1_dup2(
      FILE_PATH_LITERAL("news.google.com-Google_News_Page (2).mhtml"));
  base::FilePath actual_1_dup2(
      OfflinePageMHTMLArchiver::GenerateFileName(archives_dir, url_1, title_1));
  EXPECT_EQ(0, base::WriteFile(actual_1_dup2, NULL, 0));
  EXPECT_EQ(archives_dir.Append(expected_1_dup2), actual_1_dup2);

  // Try another url and title.
  GURL url_2("https://en.m.wikipedia.org/Sample_page_about_stuff");
  std::string title_2("Some Wiki Page");
  base::FilePath expected_2(
      FILE_PATH_LITERAL("en.m.wikipedia.org-Some_Wiki_Page.mhtml"));
  base::FilePath actual_2(
      OfflinePageMHTMLArchiver::GenerateFileName(archives_dir, url_2, title_2));
  EXPECT_EQ(0, base::WriteFile(actual_2, NULL, 0));
  EXPECT_EQ(archives_dir.Append(expected_2), actual_2);

  // Try a long title.
  GURL url_3("https://www.google.com/search");
  std::string title_3 =
      "A really really really really long long^ - TRUNCATE THIS PART ";
  std::string expected_title_3_part =
      "A_really_really_really_really_long_long^";
  base::FilePath expected_3(
      FILE_PATH_LITERAL("www.google.com-" + expected_title_3_part + ".mhtml"));
  base::FilePath actual_3(
      OfflinePageMHTMLArchiver::GenerateFileName(archives_dir, url_3, title_3));
  EXPECT_EQ(0, base::WriteFile(actual_3, NULL, 0));
  EXPECT_EQ(archives_dir.Append(expected_3), actual_3);

  // Try the long title again, to see suffix.
  base::FilePath expected_3_dup(FILE_PATH_LITERAL(
      "www.google.com-" + expected_title_3_part + " (1).mhtml"));
  base::FilePath actual_3_dup(
      OfflinePageMHTMLArchiver::GenerateFileName(archives_dir, url_3, title_3));
  EXPECT_EQ(0, base::WriteFile(actual_3_dup, NULL, 0));
  EXPECT_EQ(archives_dir.Append(expected_3_dup), actual_3_dup);
}

}  // namespace offline_pages
