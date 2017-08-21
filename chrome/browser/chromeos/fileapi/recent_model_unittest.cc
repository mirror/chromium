// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <algorithm>
#include <string>
#include <utility>

#include "base/files/file_path.h"
#include "base/memory/ptr_util.h"
#include "base/test/histogram_tester.h"
#include "chrome/browser/chromeos/fileapi/recent_context.h"
#include "chrome/browser/chromeos/fileapi/recent_file.h"
#include "chrome/browser/chromeos/fileapi/recent_model.h"
#include "chrome/browser/chromeos/fileapi/recent_model_factory.h"
#include "chrome/browser/chromeos/fileapi/test/fake_recent_source.h"
#include "chrome/test/base/testing_profile.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "storage/browser/fileapi/file_system_url.h"
#include "storage/common/fileapi/file_system_types.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace chromeos {

class RecentModelTest : public testing::Test {
 public:
  RecentModelTest() = default;

 protected:
  std::vector<RecentFile> BuildModelAndGetRecentFiles(
      std::vector<std::unique_ptr<RecentSource>> sources) {
    RecentModel* model = RecentModelFactory::SetForProfileAndUseForTest(
        &profile_, RecentModel::CreateForTest(std::move(sources)));

    std::vector<RecentFile> files_out;

    base::RunLoop run_loop;

    model->GetRecentFiles(
        RecentContext(nullptr /* file_system_context */, GURL() /* origin */),
        base::BindOnce(
            [](base::RunLoop* run_loop, std::vector<RecentFile>* files_out,
               const std::vector<RecentFile>& files) {
              *files_out = files;
              run_loop->Quit();
            },
            &run_loop, &files_out));

    run_loop.Run();

    return files_out;
  }

  content::TestBrowserThreadBundle thread_bundle_;
  TestingProfile profile_;
};

RecentFile MakeRecentFile(const std::string& name,
                          const base::Time& last_modified) {
  storage::FileSystemURL url = storage::FileSystemURL::CreateForTest(
      GURL(),  // origin
      storage::kFileSystemTypeNativeLocal, base::FilePath(name));
  return RecentFile(url, last_modified);
}

TEST_F(RecentModelTest, GetRecentFiles) {
  auto source1 = base::MakeUnique<FakeRecentSource>();
  source1->AddFile(MakeRecentFile("aaa.jpg", base::Time::FromJavaTime(1)));
  source1->AddFile(MakeRecentFile("ccc.jpg", base::Time::FromJavaTime(3)));

  auto source2 = base::MakeUnique<FakeRecentSource>();
  source2->AddFile(MakeRecentFile("bbb.jpg", base::Time::FromJavaTime(2)));
  source2->AddFile(MakeRecentFile("ddd.jpg", base::Time::FromJavaTime(4)));

  std::vector<std::unique_ptr<RecentSource>> sources;
  sources.emplace_back(std::move(source1));
  sources.emplace_back(std::move(source2));

  std::vector<RecentFile> files =
      BuildModelAndGetRecentFiles(std::move(sources));

  ASSERT_EQ(4u, files.size());
  EXPECT_EQ("ddd.jpg", files[0].url().path().value());
  EXPECT_EQ(base::Time::FromJavaTime(4), files[0].last_modified());
  EXPECT_EQ("ccc.jpg", files[1].url().path().value());
  EXPECT_EQ(base::Time::FromJavaTime(3), files[1].last_modified());
  EXPECT_EQ("bbb.jpg", files[2].url().path().value());
  EXPECT_EQ(base::Time::FromJavaTime(2), files[2].last_modified());
  EXPECT_EQ("aaa.jpg", files[3].url().path().value());
  EXPECT_EQ(base::Time::FromJavaTime(1), files[3].last_modified());
}

TEST_F(RecentModelTest, GetRecentFiles_UmaStats) {
  base::HistogramTester histogram_tester;

  BuildModelAndGetRecentFiles({});

  histogram_tester.ExpectTotalCount(RecentModel::kLoadHistogramName, 1);
}

}  // namespace chromeos
