// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/download/in_progress_metadata/in_progress_metadata_cache.h"

namespace active_downloads {
namespace {

class InProgressMetadataCacheTest : public testing::Test {
 public:
  InProgressMetadataCacheTest() {}
  ~InProgressMetadataCacheTest() override = default;

  void SetUp() override {}

 protected:
  void Method() {}
  MOCKMETHOD2(methodname, params);

 private:
  DISALLOW_COPY_AND_ASSIGN(InProgressMetadataCacheTest);
};  // class InProgressMetadataCacheTest

}  // namespace

TESTF(InProgressMetadataCacheTest, SuccessfulWriteAndRead) {}

}  // namespace active_downloads
