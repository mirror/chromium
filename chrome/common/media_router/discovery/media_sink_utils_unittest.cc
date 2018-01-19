// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/common/media_router/discovery/media_sink_utils.h"

#include <vector>
#include "testing/gtest/include/gtest/gtest.h"

namespace media_router {

TEST(MediaSinkUtilsTest, TestGenerateSameId) {
  MediaSinkUtils sink_utils;
  std::string unique_id = "uuid:de51d949-21f1-5f8a-f6db-f65592bb3610";
  std::string sink_id1 = sink_utils.GenerateId(unique_id);
  std::string sink_id2 = sink_utils.GenerateId(unique_id);
  EXPECT_EQ(sink_id1, sink_id2);
}

TEST(MediaSinkUtilsTest, TestGenerateDifferentId) {
  MediaSinkUtils sink_utils;
  std::string unique_id1 = "uuid:de51d949-21f1-5f8a-f6db-f65592bb3610";
  std::string unique_id2 = "uuid:2a58c04a-13c2-72fa-2253-a53116a5037d";
  std::string sink_id1 = sink_utils.GenerateId(unique_id1);
  std::string sink_id2 = sink_utils.GenerateId(unique_id2);
  EXPECT_NE(sink_id1, sink_id2);
}

TEST(MediaSinkUtilsTest, TestDifferentUUIDFormat) {
  MediaSinkUtils sink_utils;
  std::string unique_ids[] = {"uuid:de51d949-21f1-5f8a-f6db-f65592bb3610",
                              "de51d94921f1-5f8a-f6db-f65592bb3610",
                              "DE51D94921F15F8AF6DBF65592BB3610",
                              "abc:de51d949-21f1-5f8a-f6db-f65592bb3610", ""};

  size_t len = 5u;
  std::string sink_ids[len];
  for (size_t i = 0; i < len; i++)
    sink_ids[i] = sink_utils.GenerateId(unique_ids[i]);

  EXPECT_EQ(sink_ids[0], sink_ids[1]);
  EXPECT_EQ(sink_ids[0], sink_ids[2]);
  EXPECT_NE(sink_ids[0], sink_ids[3]);
  EXPECT_NE(sink_ids[0], sink_ids[4]);
  EXPECT_NE(sink_ids[3], sink_ids[4]);
}

TEST(MediaSinkUtilsTest, TestDifferentTokenGenerateDifferntId) {
  MediaSinkUtils sink_utils1("abc");
  MediaSinkUtils sink_utils2("def");
  std::string unique_id = "uuid:de51d949-21f1-5f8a-f6db-f65592bb3610";
  std::string sink_id1 = sink_utils1.GenerateId(unique_id);
  std::string sink_id2 = sink_utils2.GenerateId(unique_id);
  EXPECT_NE(sink_id1, sink_id2);
}

}  // namespace media_router
