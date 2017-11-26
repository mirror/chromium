// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/message_loop/message_loop.h"
#include "mojo/public/cpp/bindings/binding_set.h"
#include "services/network/public/cpp/mutable_network_traffic_annotation_tag_struct_traits.h"
#include "services/network/public/cpp/mutable_network_traffic_annotation_tag_traits_test_service.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace network {

namespace {

class MutableNetworkTrafficAnnottionTagsTest
    : public testing::Test,
      public mojom::MutableNetworkTrafficAnnotationTagTestService {
 protected:
  MutableNetworkTrafficAnnottionTagsTest() = default;

  mojom::MutableNetworkTrafficAnnotationTagTestServicePtr GetTestProxy() {
    mojom::MutableNetworkTrafficAnnotationTagTestServicePtr proxy;
    test_bindings_.AddBinding(this, mojo::MakeRequest(&proxy));
    return proxy;
  }

 private:
  // MutableNetworkTrafficAnnotationTagTestService:
  void EchoMutableNetworkTrafficAnnotationTag(
      const net::MutableNetworkTrafficAnnotationTag& tag,
      EchoMutableNetworkTrafficAnnotationTagCallback callback) override {
    std::move(callback).Run(tag);
  }

  base::MessageLoop loop_;
  mojo::BindingSet<MutableNetworkTrafficAnnotationTagTestService>
      test_bindings_;
  DISALLOW_COPY_AND_ASSIGN(MutableNetworkTrafficAnnottionTagsTest);
};

}  // namespace

TEST_F(MutableNetworkTrafficAnnottionTagsTest, BasicTest) {
  net::MutableNetworkTrafficAnnotationTag input;
  net::MutableNetworkTrafficAnnotationTag output;

  mojom::MutableNetworkTrafficAnnotationTagTestServicePtr proxy =
      GetTestProxy();

  input.unique_id_hash_code = 1;
  proxy->EchoMutableNetworkTrafficAnnotationTag(input, &output);
  EXPECT_EQ(output.unique_id_hash_code, 1);

  input.unique_id_hash_code = 2;
  proxy->EchoMutableNetworkTrafficAnnotationTag(input, &output);
  EXPECT_EQ(output.unique_id_hash_code, 2);
}

}  // namespace network
