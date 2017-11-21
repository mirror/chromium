// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string.h>

#include <algorithm>

#include "net/cert/root_cert_list_generated.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace net {

namespace {

TEST(KnownRootsTest, RootCertDataIsSorted) {
  EXPECT_TRUE(std::is_sorted(
      std::begin(kRootCerts), std::end(kRootCerts),
      [](const RootCertData& lhs, const RootCertData& rhs) {
        return memcmp(lhs.sha256_spki_hash, rhs.sha256_spki_hash, 32) < 0;
      }));
}

}  // namespace

}  // namespace net
