// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/model/move_page_task.h"

#include "base/files/file_util.h"
#include "base/test/test_mock_time_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "components/offline_pages/core/model/offline_page_item_generator.h"
#include "components/offline_pages/core/offline_page_metadata_store_test_util.h"
#include "components/offline_pages/core/task.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace offline_pages {

class MovePageTaskTest : public testing::Test {
 public:
  MovePageTaskTest();
  ~MovePageTaskTest() override;

  void SetUp() override;
  void TearDown() override;

  OfflinePageMetadataStoreSQL* store() { return store_test_util_.store(); }

  OfflinePageMetadataStoreTestUtil* store_test_util() {
    return &store_test_util_;
  }

  OfflinePageItemGenerator* generator() { return &generator_; }

  scoped_refptr<base::TestMockTimeTaskRunner> task_runner() {
    return task_runner_.get();
  }

  void MoveTaskDone(Task* task) { move_task_done_ = true; }

  bool move_task_done() { return move_task_done_; }

  base::FilePath& found_new_path() { return found_new_path_; }

  base::FilePath& expected_file_path() { return expected_file_path_; }

  OfflinePageItem& offline_page() { return offline_page_; }

  base::ScopedTempDir& private_temp_dir() { return private_temp_dir_; }

  base::ScopedTempDir& public_temp_dir() { return public_temp_dir_; }

  void PumpLoop();

  void SetTaskCompletionCallbackForTesting(Task* task);

  void MoveDoneCallback(const OfflinePageItem& page,
                        const base::FilePath& new_file_path);

 private:
  scoped_refptr<base::TestMockTimeTaskRunner> task_runner_;
  base::ThreadTaskRunnerHandle task_runner_handle_;
  OfflinePageMetadataStoreTestUtil store_test_util_;
  OfflinePageItemGenerator generator_;
  OfflinePageItem offline_page_;
  bool move_task_done_;
  base::FilePath found_new_path_;
  base::FilePath expected_file_path_;
  base::ScopedTempDir private_temp_dir_;
  base::ScopedTempDir public_temp_dir_;
};

MovePageTaskTest::MovePageTaskTest()
    : task_runner_(new base::TestMockTimeTaskRunner()),
      task_runner_handle_(task_runner_),
      store_test_util_(task_runner_),
      move_task_done_(false) {}

MovePageTaskTest::~MovePageTaskTest() {}

void MovePageTaskTest::SetUp() {
  store_test_util_.BuildStoreInMemory();
  // Set up a public and a private dir.
  ASSERT_TRUE(private_temp_dir_.CreateUniqueTempDir());
  ASSERT_TRUE(public_temp_dir_.CreateUniqueTempDir());
  generator()->SetArchiveDirectory(private_temp_dir_.GetPath());
  offline_page_ = generator()->CreateItemWithTempFile();

  // The full path name we expect to find after the file move.
  expected_file_path_ =
      public_temp_dir_.GetPath().Append(offline_page_.file_path.BaseName());
}

void MovePageTaskTest::TearDown() {
  store_test_util_.DeleteStore();
  // Cleanup the file from the public and private dir.
  if (private_temp_dir_.IsValid()) {
    if (!private_temp_dir_.Delete())
      DVLOG(1) << "private_temp_dir_ not created";
  }
  if (public_temp_dir_.IsValid()) {
    if (!public_temp_dir_.Delete())
      DVLOG(1) << "public_temp_dir_ not created";
  }
}

void MovePageTaskTest::PumpLoop() {
  task_runner_->RunUntilIdle();
}

void MovePageTaskTest::MoveDoneCallback(const OfflinePageItem& page,
                                        const base::FilePath& new_file_path) {
  found_new_path_ = new_file_path;
}

void MovePageTaskTest::SetTaskCompletionCallbackForTesting(Task* task) {
  task->SetTaskCompletionCallbackForTesting(
      task_runner_.get(), base::BindRepeating(&MovePageTaskTest::MoveTaskDone,
                                              base::Unretained(this)));
}

TEST_F(MovePageTaskTest, MovePageTest) {
  store_test_util()->InsertItem(offline_page());

  MovePageCallback done_callback = base::BindOnce(
      &MovePageTaskTest::MoveDoneCallback, base::Unretained(this));

  auto task = std::make_unique<MovePageTask>(store(), offline_page(),
                                             public_temp_dir().GetPath(),
                                             std::move(done_callback));
  SetTaskCompletionCallbackForTesting(task.get());
  task->Run();
  PumpLoop();

  // Check that task finished running.
  ASSERT_TRUE(move_task_done());

  // Check the download ID got set in the offline page item in the database.
  auto found_offline_page =
      store_test_util()->GetPageByOfflineId(offline_page().offline_id);
  ASSERT_TRUE(found_offline_page);
  EXPECT_EQ(found_offline_page->file_path, expected_file_path());

  // Assert test file actually got moved to a new test directory.
  ASSERT_TRUE(base::PathExists(expected_file_path()));

  // Ensure done callback got called, moved file path is what we expect.
  EXPECT_EQ(found_new_path(), expected_file_path());
}

TEST_F(MovePageTaskTest, MovingPageFails) {
  store_test_util()->InsertItem(offline_page());

  MovePageCallback done_callback = base::BindOnce(
      &MovePageTaskTest::MoveDoneCallback, base::Unretained(this));

  base::FilePath emptyFilePath;

  auto task = std::make_unique<MovePageTask>(
      store(), offline_page(), emptyFilePath, std::move(done_callback));
  SetTaskCompletionCallbackForTesting(task.get());
  task->Run();
  PumpLoop();

  // Check that task finished running.
  ASSERT_TRUE(move_task_done());

  // Check the download ID got set in the offline page item in the database.
  auto found_offline_page =
      store_test_util()->GetPageByOfflineId(offline_page().offline_id);
  ASSERT_TRUE(found_offline_page);
  EXPECT_NE(found_offline_page->file_path, expected_file_path());

  // Assert test file actually got moved to a new test directory.
  ASSERT_FALSE(base::PathExists(expected_file_path()));
}

TEST_F(MovePageTaskTest, MoveSucceededDBUpdateFailed) {
  MovePageCallback done_callback = base::BindOnce(
      &MovePageTaskTest::MoveDoneCallback, base::Unretained(this));

  auto task = std::make_unique<MovePageTask>(store(), offline_page(),
                                             public_temp_dir().GetPath(),
                                             std::move(done_callback));
  SetTaskCompletionCallbackForTesting(task.get());
  task->Run();
  PumpLoop();

  // Check that task finished running.
  ASSERT_TRUE(move_task_done());

  // Check the download ID got set in the offline page item in the database.
  auto found_offline_page =
      store_test_util()->GetPageByOfflineId(offline_page().offline_id);
  ASSERT_EQ(found_offline_page.get(), reinterpret_cast<OfflinePageItem*>(NULL));

  // Assert test file actually got moved to a new test directory.
  ASSERT_TRUE(base::PathExists(expected_file_path()));

  // Ensure done callback got called, moved file path is what we expect.
  EXPECT_EQ(found_new_path(), expected_file_path());
}

}  // namespace offline_pages
