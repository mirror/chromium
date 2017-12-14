// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/interfaces/ip_endpoint_struct_traits.h"

#include "net/base/ip_address.h"
#include "net/base/ip_endpoint.h"
#include "net/interfaces/ip_address.mojom.h"
#include "net/interfaces/ip_address_struct_traits.h"
#include "net/interfaces/ip_endpoint.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace net {

TEST(IPEndPointStructTraits, BasicTest) {
  // Serializing and Deserializing an empty IPEndPoint is okay.
  {
    IPEndPoint empty;
    IPEndPoint output;

    EXPECT_TRUE(interfaces::IPEndPoint::Deserialize(
        interfaces::IPEndPoint::Serialize(&empty), &output));
    EXPECT_EQ(empty, output);
  }

  // Serializing and Deserializing an IPEndPoint that is invalid (neither ipv4
  // or ipv6). This should not result in a serialization error.
  {
    std::vector<uint8_t> invalid_ip{127, 0, 0, 0, 1};  // 5 digits instead of 4.
    IPAddress invalid(invalid_ip.data(), invalid_ip.size());
    EXPECT_FALSE(invalid.IsValid());
    IPEndPoint invalid_endpoint(invalid, 0);
    IPEndPoint output;

    EXPECT_TRUE(interfaces::IPEndPoint::Deserialize(
        interfaces::IPEndPoint::Serialize(&invalid_endpoint), &output));
    EXPECT_EQ(invalid_endpoint, output);
  }
}

}  // namespace net
