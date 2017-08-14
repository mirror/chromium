// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/prefetch/download_archives_task.h"

#include <algorithm>
#include <set>

#include "components/offline_pages/core/prefetch/store/prefetch_store_test_util.h"
#include "components/offline_pages/core/prefetch/store/prefetch_store_utils.h"
#include "components/offline_pages/core/prefetch/task_test_base.h"
#include "components/offline_pages/core/prefetch/test_prefetch_downloader.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace offline_pages {

namespace {

const char kTestArchiveBodyName1[] = "test_archive_body_name_1";
const char kTestArchiveBodyName2[] = "test_archive_body_name_2";

const PrefetchItem* FindPrefetchItemByOfflineId(
    const std::set<PrefetchItem>& items,
    int64_t offline_id) {
  auto found_it = std::find_if(items.begin(), items.end(),
                               [&offline_id](const PrefetchItem& i) -> bool {
                                 return i.offline_id == offline_id;
                               });
  if (found_it != items.end())
    return &(*found_it);
  return nullptr;
}

class DownloadArchivesTaskTest : public TaskTestBase {
 public:
  TestPrefetchDownloader* prefetch_downloader() {
    return &test_prefetch_downloader_;
  }

  int64_t InsertDummyItem();
  int64_t InsertItemToDownload(const std::string& archive_body_name);

 private:
  TestPrefetchDownloader test_prefetch_downloader_;
};

int64_t DownloadArchivesTaskTest::InsertDummyItem() {
  PrefetchItem item;
  item.offline_id = PrefetchStoreUtils::GenerateOfflineId();
  store_util()->InsertPrefetchItem(item);
  return item.offline_id;
}

int64_t DownloadArchivesTaskTest::InsertItemToDownload(
    const std::string& archive_body_name) {
  PrefetchItem item;
  item.offline_id = PrefetchStoreUtils::GenerateOfflineId();
  item.state = PrefetchItemState::RECEIVED_BUNDLE;
  item.archive_body_name = archive_body_name;
  store_util()->InsertPrefetchItem(item);
  return item.offline_id;
}

TEST_F(DownloadArchivesTaskTest, NoArchivesToDownload) {
  InsertDummyItem();

  std::set<PrefetchItem> items_before_run;
  EXPECT_EQ(1U, store_util()->GetAllItems(&items_before_run));

  DownloadArchivesTask task(store(), prefetch_downloader());
  ExpectTaskCompletes(&task);
  task.Run();
  RunUntilIdle();

  std::set<PrefetchItem> items_after_run;
  EXPECT_EQ(1U, store_util()->GetAllItems(&items_after_run));

  EXPECT_EQ(items_before_run, items_after_run);
}

TEST_F(DownloadArchivesTaskTest, SingleArchiveToDownload) {
  int64_t dummy_item_id = InsertDummyItem();
  int64_t download_item_id = InsertItemToDownload(kTestArchiveBodyName1);

  std::set<PrefetchItem> items_before_run;
  EXPECT_EQ(2U, store_util()->GetAllItems(&items_before_run));

  DownloadArchivesTask task(store(), prefetch_downloader());
  ExpectTaskCompletes(&task);
  task.Run();
  RunUntilIdle();

  std::set<PrefetchItem> items_after_run;
  EXPECT_EQ(2U, store_util()->GetAllItems(&items_after_run));

  const PrefetchItem* dummy_item_before =
      FindPrefetchItemByOfflineId(items_before_run, dummy_item_id);
  const PrefetchItem* dummy_item_after =
      FindPrefetchItemByOfflineId(items_after_run, dummy_item_id);
  EXPECT_TRUE(dummy_item_before);
  EXPECT_TRUE(dummy_item_after);
  EXPECT_EQ(*dummy_item_before, *dummy_item_after);

  const PrefetchItem* download_item =
      FindPrefetchItemByOfflineId(items_after_run, download_item_id);
  EXPECT_TRUE(download_item);
  EXPECT_EQ(PrefetchItemState::DOWNLOADING, download_item->state);

  std::map<std::string, std::string> requested_downloads =
      prefetch_downloader()->requested_downloads();
  auto it = requested_downloads.find(download_item->guid);
  EXPECT_TRUE(it != requested_downloads.end());
  EXPECT_EQ(it->second, download_item->archive_body_name);
}

TEST_F(DownloadArchivesTaskTest, MultipleArchivesToDownload) {
  int64_t dummy_item_id = InsertDummyItem();
  int64_t download_item_id_1 = InsertItemToDownload(kTestArchiveBodyName1);
  int64_t download_item_id_2 = InsertItemToDownload(kTestArchiveBodyName2);

  std::set<PrefetchItem> items_before_run;
  EXPECT_EQ(3U, store_util()->GetAllItems(&items_before_run));

  DownloadArchivesTask task(store(), prefetch_downloader());
  ExpectTaskCompletes(&task);
  task.Run();
  RunUntilIdle();

  std::set<PrefetchItem> items_after_run;
  EXPECT_EQ(3U, store_util()->GetAllItems(&items_after_run));

  const PrefetchItem* dummy_item_before =
      FindPrefetchItemByOfflineId(items_before_run, dummy_item_id);
  const PrefetchItem* dummy_item_after =
      FindPrefetchItemByOfflineId(items_after_run, dummy_item_id);
  EXPECT_TRUE(dummy_item_before);
  EXPECT_TRUE(dummy_item_after);
  EXPECT_EQ(*dummy_item_before, *dummy_item_after);

  const PrefetchItem* download_item_1 =
      FindPrefetchItemByOfflineId(items_after_run, download_item_id_1);
  EXPECT_TRUE(download_item_1);
  EXPECT_EQ(PrefetchItemState::DOWNLOADING, download_item_1->state);

  const PrefetchItem* download_item_2 =
      FindPrefetchItemByOfflineId(items_after_run, download_item_id_2);
  EXPECT_TRUE(download_item_2);
  EXPECT_EQ(PrefetchItemState::DOWNLOADING, download_item_2->state);

  std::map<std::string, std::string> requested_downloads =
      prefetch_downloader()->requested_downloads();
  EXPECT_EQ(2U, requested_downloads.size());

  auto it = requested_downloads.find(download_item_1->guid);
  EXPECT_TRUE(it != requested_downloads.end());
  EXPECT_EQ(it->second, download_item_1->archive_body_name);

  it = requested_downloads.find(download_item_2->guid);
  EXPECT_TRUE(it != requested_downloads.end());
  EXPECT_EQ(it->second, download_item_2->archive_body_name);
}

}  // namespace
}  // namespace offline_pages
