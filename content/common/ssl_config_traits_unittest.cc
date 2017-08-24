// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/common/ssl_config_traits.h"

#include "content/public/common/ssl_config.mojom.h"
#include "net/ssl/ssl_config.h"
#include "net/ssl/ssl_config_service.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace content {
namespace {

testing::AssertionResult TestRoundTrip(const net::SSLConfig& original) {
  net::SSLConfig copied;
  if (!mojom::SSLConfig::Deserialize(mojom::SSLConfig::Serialize(&original),
                                     &copied)) {
    return testing::AssertionFailure() << "Deserialization failure";
  }

  // tls13_variant has to be checked individually because SSLConfigService does
  // not consider it part of the user configuration.
  return testing::AssertionResult(
             net::SSLConfigService::UserConfigsAreEqualForTesting(original,
                                                                  copied) &&
             original.tls13_variant == copied.tls13_variant)
         << "Config mismatch";
}

// Test each field that should be copied, one-at-a-time.
TEST(SSLConfigTraits, ThereAndBackAgain) {
  net::SSLConfig config;
  // Test default configuration first, since if it fails, most other tests are
  // likely to fail as well.
  EXPECT_TRUE(TestRoundTrip(config));

  config.rev_checking_enabled = false;
  EXPECT_TRUE(TestRoundTrip(config));
  config.rev_checking_enabled = true;
  EXPECT_TRUE(TestRoundTrip(config));

  config.rev_checking_required_local_anchors = false;
  EXPECT_TRUE(TestRoundTrip(config));
  config.rev_checking_required_local_anchors = true;
  EXPECT_TRUE(TestRoundTrip(config));

  config.sha1_local_anchors_enabled = false;
  EXPECT_TRUE(TestRoundTrip(config));
  config.sha1_local_anchors_enabled = true;
  EXPECT_TRUE(TestRoundTrip(config));

  config.common_name_fallback_local_anchors_enabled = false;
  EXPECT_TRUE(TestRoundTrip(config));
  config.common_name_fallback_local_anchors_enabled = true;
  EXPECT_TRUE(TestRoundTrip(config));

  config.version_min = net::SSL_PROTOCOL_VERSION_TLS1_2;
  config.version_max = net::SSL_PROTOCOL_VERSION_TLS1_2;
  EXPECT_TRUE(TestRoundTrip(config));
  config.version_max = net::SSL_PROTOCOL_VERSION_TLS1_3;
  EXPECT_TRUE(TestRoundTrip(config));
  config.version_min = net::SSL_PROTOCOL_VERSION_TLS1_3;
  config.version_max = net::SSL_PROTOCOL_VERSION_TLS1_3;
  EXPECT_TRUE(TestRoundTrip(config));

  config.tls13_variant = net::kTLS13VariantDraft;
  EXPECT_TRUE(TestRoundTrip(config));
  config.tls13_variant = net::kTLS13VariantExperiment;
  EXPECT_TRUE(TestRoundTrip(config));

  config.disabled_cipher_suites.push_back(0x0004);
  EXPECT_TRUE(TestRoundTrip(config));
  config.disabled_cipher_suites.push_back(0xC002);
  EXPECT_TRUE(TestRoundTrip(config));

  config.channel_id_enabled = false;
  EXPECT_TRUE(TestRoundTrip(config));
  config.channel_id_enabled = true;
  EXPECT_TRUE(TestRoundTrip(config));

  config.false_start_enabled = false;
  EXPECT_TRUE(TestRoundTrip(config));
  config.false_start_enabled = true;
  EXPECT_TRUE(TestRoundTrip(config));

  config.require_ecdhe = false;
  EXPECT_TRUE(TestRoundTrip(config));
  config.require_ecdhe = true;
  EXPECT_TRUE(TestRoundTrip(config));
}

}  // namespace
}  // namespace content
