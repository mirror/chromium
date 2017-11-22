// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/nqe/network_id.h"

#include <string>

#include "base/strings/string_number_conversions.h"
#include "net/base/network_change_notifier.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace net {
namespace nqe {
namespace internal {

namespace {

const char kValueSeparator = ',';

TEST(NetworkIDTest, TestSerialize) {
  nqe::internal::NetworkID network_id(NetworkChangeNotifier::CONNECTION_2G,
                                      "test1", 2);
  std::string serialized = network_id.ToString();
  // The serialized string should not contain a period since the network ID
  // is used as a key in the dictionary pref, and the keys are prohibited from
  // containing periods.
  EXPECT_EQ(std::string::npos, serialized.find('.'));
  EXPECT_EQ(network_id, NetworkID::FromString(serialized));
}

// Tests that the serialized network IDs with missing signal strength are
// read correctly, and the signal strength is set to the default value of 0.
TEST(NetworkIDTest, TestNoSignalStrength) {
  nqe::internal::NetworkID network_id(NetworkChangeNotifier::CONNECTION_2G,
                                      "test1", 0);

  // Serialized network ID does not have the signal strength present.
  std::string serialized = network_id.id + kValueSeparator +
                           base::IntToString(static_cast<int>(network_id.type));
  EXPECT_EQ(network_id, NetworkID::FromString(serialized));
}

#if defined(NDEBUG) && !defined(DCHECK_ALWAYS_ON)
TEST(NetworkIDTest, TestNoSeparator) {
  nqe::internal::NetworkID network_id(NetworkChangeNotifier::CONNECTION_UNKNOWN,
                                      "", 0);

  // Invalid serialized network ID may crash on debug builds.
  std::string serialized = "test1";
  EXPECT_EQ(network_id, NetworkID::FromString(serialized));
}
#endif  // defined(NDEBUG) && !defined(DCHECK_ALWAYS_ON)

}  // namespace

}  // namespace internal
}  // namespace nqe
}  // namespace net
