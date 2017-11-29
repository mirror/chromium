// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/macros.h"
#include "base/message_loop/message_loop.h"
#include "mojo/public/cpp/bindings/binding_set.h"
#include "services/network/public/cpp/mutable_network_traffic_annotation_tag_struct_traits.h"
#include "services/network/public/interfaces/mutable_network_traffic_annotation_tag.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace network {

namespace {

class MutableNetworkTrafficAnnottionTagsTest : public testing::Test {
 protected:
  MutableNetworkTrafficAnnottionTagsTest() = default;

  DISALLOW_COPY_AND_ASSIGN(MutableNetworkTrafficAnnottionTagsTest);
};

}  // namespace

TEST_F(MutableNetworkTrafficAnnottionTagsTest, BasicTest) {
  net::MutableNetworkTrafficAnnotationTag original;
  net::MutableNetworkTrafficAnnotationTag copy;

  original.unique_id_hash_code = 1;
  EXPECT_TRUE(mojom::MutableNetworkTrafficAnnotationTag::Deserialize(
      mojom::MutableNetworkTrafficAnnotationTag::Serialize(&original), &copy));
  EXPECT_EQ(copy.unique_id_hash_code, 1);

  original.unique_id_hash_code = 2;
  EXPECT_TRUE(mojom::MutableNetworkTrafficAnnotationTag::Deserialize(
      mojom::MutableNetworkTrafficAnnotationTag::Serialize(&original), &copy));
  EXPECT_EQ(copy.unique_id_hash_code, 2);
}

}  // namespace network
