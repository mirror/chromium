// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/message_loop/message_loop.h"
#include "mojo/public/cpp/bindings/binding_set.h"
#include "services/network/public/cpp/mutable_partial_network_traffic_annotation_tag_struct_traits.h"
#include "services/network/public/cpp/mutable_partial_network_traffic_annotation_tag_traits_test_service.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace network {

namespace {

class MutablePartialNetworkTrafficAnnottionTagsTest
    : public testing::Test,
      public mojom::MutablePartialNetworkTrafficAnnotationTagTestService {
 protected:
  MutablePartialNetworkTrafficAnnottionTagsTest() = default;

  mojom::MutablePartialNetworkTrafficAnnotationTagTestServicePtr
  GetTestProxy() {
    mojom::MutablePartialNetworkTrafficAnnotationTagTestServicePtr proxy;
    test_bindings_.AddBinding(this, mojo::MakeRequest(&proxy));
    return proxy;
  }

 private:
  // MutablePartialNetworkTrafficAnnotationTagTestService:
  void EchoMutablePartialNetworkTrafficAnnotationTag(
      const net::MutablePartialNetworkTrafficAnnotationTag& tag,
      EchoMutablePartialNetworkTrafficAnnotationTagCallback callback) override {
    std::move(callback).Run(tag);
  }

  base::MessageLoop loop_;
  mojo::BindingSet<MutablePartialNetworkTrafficAnnotationTagTestService>
      test_bindings_;
  DISALLOW_COPY_AND_ASSIGN(MutablePartialNetworkTrafficAnnottionTagsTest);
};

}  // namespace

TEST_F(MutablePartialNetworkTrafficAnnottionTagsTest, BasicTest) {
  net::MutablePartialNetworkTrafficAnnotationTag input;
  net::MutablePartialNetworkTrafficAnnotationTag output;

  mojom::MutablePartialNetworkTrafficAnnotationTagTestServicePtr proxy =
      GetTestProxy();

  input.unique_id_hash_code = 1;
  input.completing_id_hash_code = 2;
  proxy->EchoMutablePartialNetworkTrafficAnnotationTag(input, &output);
  EXPECT_EQ(output.unique_id_hash_code, 1);
  EXPECT_EQ(output.completing_id_hash_code, 2);
}

}  // namespace network
