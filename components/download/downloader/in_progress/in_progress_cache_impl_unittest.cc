// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/download/downloader/in_progress/in_progress_cache_impl.h"

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/memory/ptr_util.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using testing::_;

namespace download {
namespace {

// TODO(jming): Check that this makes sense.
// const base::FilePath::CharType kInProgressCachePath[] =
// FILE_PATH_LITERAL("/test/in_progress_cache");

class InProgressCacheImplTest : public testing::Test {
 public:
  InProgressCacheImplTest() : cache_initialized_(false) {}
  ~InProgressCacheImplTest() override = default;

  void SetUp() override {
    base::FilePath file_path;
    scoped_refptr<base::SequencedTaskRunner> task_runner;
    cache_ = base::MakeUnique<InProgressCacheImpl>(file_path, task_runner);
  }

 protected:
  void InitializeCache() {
    cache_->Initialize(base::Bind(&InProgressCacheImplTest::OnCacheInitialized,
                                  base::Unretained(this)));
  }

  void OnCacheInitialized() { cache_initialized_ = true; }

  std::unique_ptr<InProgressCacheImpl> cache_;
  bool cache_initialized_;

 private:
  DISALLOW_COPY_AND_ASSIGN(InProgressCacheImplTest);
};  // class InProgressCacheImplTest

}  // namespace

TEST_F(InProgressCacheImplTest, Initialize) {
  // TOOD(jming): Expect calls?

  InitializeCache();
  EXPECT_TRUE(cache_initialized_);
}

}  // namespace download
