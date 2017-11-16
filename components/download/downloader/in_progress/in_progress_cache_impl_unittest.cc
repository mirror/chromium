// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/download/downloader/in_progress/in_progress_cache_impl.h"

using testing::_;

namespace download {
namespace {

class InProgressCacheTest : public testing::Test {
 public:
  InProgressCacheTest() {}
  ~InProgressCacheTest() override = default;

  void SetUp() override {}

 protected:
 private:
  DISALLOW_COPY_AND_ASSIGN(InProgressCacheTest);
}  // class InProgressCacheTest

}  // namespace

TEST_F(InProgressCacheTest, SuccessfulWriteAndRead) {}

}  // namespace download
