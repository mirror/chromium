// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <utility>

#include "base/logging.h"
#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/time/time.h"
#include "chrome/browser/chromeos/fileapi/recent_context.h"
#include "chrome/browser/chromeos/fileapi/recent_model.h"
#include "chrome/browser/chromeos/fileapi/test/fake_recent_source.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace chromeos {
namespace {

class RecentModelTest : public testing::Test {
 public:
  RecentModelTest() = default;

 protected:
  using BuildModelAndFilesMapCheckerFunction =
      void (*)(const RecentModel::FilesMap& files_map);

  void BuildModelAndFilesMap(std::vector<std::unique_ptr<RecentSource>> sources,
                             BuildModelAndFilesMapCheckerFunction checker) {
    // Limit the maximum number of files from single source to 2.
    auto model = base::MakeUnique<RecentModel>(std::move(sources), 2);

    base::RunLoop run_loop;

    model->GetFilesMap(RecentContext(), false,
                       base::BindOnce(
                           [](base::RunLoop* run_loop,
                              BuildModelAndFilesMapCheckerFunction checker,
                              const RecentModel::FilesMap& files_map) {
                             checker(files_map);
                             run_loop->Quit();
                           },
                           &run_loop, checker));

    run_loop.Run();
  }

  content::TestBrowserThreadBundle thread_bundle_;
};

TEST_F(RecentModelTest, GetFilesMap) {
  auto source1 =
      base::MakeUnique<FakeRecentSource>(RecentFile::SourceType::TEST_1);
  source1->AddFile("aaa.jpg", "a", 1, base::Time());
  source1->AddFile("bbb.jpg", "b", 2, base::Time());

  auto source2 =
      base::MakeUnique<FakeRecentSource>(RecentFile::SourceType::TEST_2);
  source2->AddFile("ccc.jpg", "c", 3, base::Time());
  source2->AddFile("ddd.jpg", "d", 4, base::Time());

  std::vector<std::unique_ptr<RecentSource>> sources;
  sources.emplace_back(std::move(source1));
  sources.emplace_back(std::move(source2));

  BuildModelAndFilesMap(std::move(sources),
                        [](const RecentModel::FilesMap& files_map) {
                          ASSERT_EQ(4u, files_map.size());

                          auto iter = files_map.begin();
                          EXPECT_EQ("aaa.jpg", iter->first.value());
                          EXPECT_EQ("a", iter->second->unique_id());
                          ++iter;
                          EXPECT_EQ("bbb.jpg", iter->first.value());
                          EXPECT_EQ("b", iter->second->unique_id());
                          ++iter;
                          EXPECT_EQ("ccc.jpg", iter->first.value());
                          EXPECT_EQ("c", iter->second->unique_id());
                          ++iter;
                          EXPECT_EQ("ddd.jpg", iter->first.value());
                          EXPECT_EQ("d", iter->second->unique_id());
                        });
}

TEST_F(RecentModelTest, GetFilesMap_Duplicates) {
  auto source1 =
      base::MakeUnique<FakeRecentSource>(RecentFile::SourceType::TEST_2);
  source1->AddFile("kitten.jpg", "2", 2, base::Time());
  source1->AddFile("kitten.jpg", "4", 4, base::Time());

  auto source2 =
      base::MakeUnique<FakeRecentSource>(RecentFile::SourceType::TEST_1);
  source2->AddFile("kitten.jpg", "3", 3, base::Time());
  source2->AddFile("kitten.jpg", "1", 1, base::Time());

  std::vector<std::unique_ptr<RecentSource>> sources;
  sources.emplace_back(std::move(source1));
  sources.emplace_back(std::move(source2));

  BuildModelAndFilesMap(std::move(sources),
                        [](const RecentModel::FilesMap& files_map) {
                          ASSERT_EQ(4u, files_map.size());

                          // We compare |source_type| and then |unique_id|, so
                          // processing order is:
                          //   1 -> 3 -> 2 -> 4
                          // However, since "kitten.jpg" comes after "kitten
                          // (N).jpg", order in FilesMap is:
                          //   3 -> 2 -> 4 -> 1
                          auto iter = files_map.begin();
                          EXPECT_EQ("kitten (1).jpg", iter->first.value());
                          EXPECT_EQ("3", iter->second->unique_id());
                          ++iter;
                          EXPECT_EQ("kitten (2).jpg", iter->first.value());
                          EXPECT_EQ("2", iter->second->unique_id());
                          ++iter;
                          EXPECT_EQ("kitten (3).jpg", iter->first.value());
                          EXPECT_EQ("4", iter->second->unique_id());
                          ++iter;
                          EXPECT_EQ("kitten.jpg", iter->first.value());
                          EXPECT_EQ("1", iter->second->unique_id());
                        });
}

TEST_F(RecentModelTest, GetFilesMap_ExceedMaxFilesFromSingleSource) {
  // Note that we limit the maxmium number of files from single source to 2
  // in this test.
  auto source1 =
      base::MakeUnique<FakeRecentSource>(RecentFile::SourceType::TEST_1);
  source1->AddFile("aaa.jpg", "a", 1, base::Time::FromTimeT(100));
  source1->AddFile("bbb.jpg", "b", 2, base::Time::FromTimeT(200));
  source1->AddFile("ccc.jpg", "c", 3, base::Time::FromTimeT(300));

  auto source2 =
      base::MakeUnique<FakeRecentSource>(RecentFile::SourceType::TEST_2);
  source2->AddFile("ddd.jpg", "d", 4, base::Time::FromTimeT(500));
  source2->AddFile("eee.jpg", "e", 5, base::Time::FromTimeT(500));
  source2->AddFile("fff.jpg", "f", 6, base::Time::FromTimeT(500));

  std::vector<std::unique_ptr<RecentSource>> sources;
  sources.emplace_back(std::move(source1));
  sources.emplace_back(std::move(source2));

  BuildModelAndFilesMap(std::move(sources),
                        [](const RecentModel::FilesMap& files_map) {
                          ASSERT_EQ(4u, files_map.size());

                          auto iter = files_map.begin();
                          EXPECT_EQ("bbb.jpg", iter->first.value());
                          EXPECT_EQ("b", iter->second->unique_id());
                          ++iter;
                          EXPECT_EQ("ccc.jpg", iter->first.value());
                          EXPECT_EQ("c", iter->second->unique_id());
                          ++iter;
                          EXPECT_EQ("ddd.jpg", iter->first.value());
                          EXPECT_EQ("d", iter->second->unique_id());
                          ++iter;
                          EXPECT_EQ("eee.jpg", iter->first.value());
                          EXPECT_EQ("e", iter->second->unique_id());
                        });
}

}  // namespace
}  // namespace chromeos
