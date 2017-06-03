// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/download/internal/file_monitor.h"

#include <algorithm>

#include "base/bind.h"
#include "base/callback.h"
#include "base/guid.h"
#include "base/memory/ptr_util.h"
#include "components/download/internal/entry.h"
#include "components/download/internal/test/entry_utils.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using testing::_;

namespace download {

class FileMonitorTest : public testing::Test {
 public:
  FileMonitorTest() {}
  ~FileMonitorTest() override = default;

 private:
  DISALLOW_COPY_AND_ASSIGN(FileMonitorTest);
};

TEST_F(FileMonitorTest, RemoveUnassociatedFiles) {
  // TODO(shaktisahu): Complete test.
}

}  // namespace download
