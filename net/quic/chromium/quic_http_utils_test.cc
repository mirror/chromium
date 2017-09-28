// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/quic/chromium/quic_http_utils.h"

#include <cstdint>
#include <limits>

#include "net/quic/platform/api/quic_endian.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace net {
namespace test {

class QuicHttpUtilsTest : public ::testing::TestWithParam<bool> {
 public:
  QuicHttpUtilsTest() {
    FLAGS_quic_reloadable_flag_quic_use_net_byte_order_version_label =
        GetParam();
  }

  uint32_t GetIetfVersionNumber(QuicVersion version) {
    uint32_t alt_svc_version = QuicVersionToQuicVersionLabel(version);
    if (!FLAGS_quic_reloadable_flag_quic_use_net_byte_order_version_label)
      alt_svc_version = QuicEndian::NetToHost32(alt_svc_version);
    return alt_svc_version;
  }
};

INSTANTIATE_TEST_CASE_P(Tests,
                        QuicHttpUtilsTest,
                        ::testing::ValuesIn({true, false}));

TEST_P(QuicHttpUtilsTest, ConvertRequestPriorityToQuicPriority) {
  EXPECT_EQ(0u, ConvertRequestPriorityToQuicPriority(HIGHEST));
  EXPECT_EQ(1u, ConvertRequestPriorityToQuicPriority(MEDIUM));
  EXPECT_EQ(2u, ConvertRequestPriorityToQuicPriority(LOW));
  EXPECT_EQ(3u, ConvertRequestPriorityToQuicPriority(LOWEST));
  EXPECT_EQ(4u, ConvertRequestPriorityToQuicPriority(IDLE));
}

TEST_P(QuicHttpUtilsTest, ConvertQuicPriorityToRequestPriority) {
  EXPECT_EQ(HIGHEST, ConvertQuicPriorityToRequestPriority(0));
  EXPECT_EQ(MEDIUM, ConvertQuicPriorityToRequestPriority(1));
  EXPECT_EQ(LOW, ConvertQuicPriorityToRequestPriority(2));
  EXPECT_EQ(LOWEST, ConvertQuicPriorityToRequestPriority(3));
  EXPECT_EQ(IDLE, ConvertQuicPriorityToRequestPriority(4));
  // These are invalid values, but we should still handle them
  // gracefully. TODO(rtenneti): should we test for all possible values of
  // uint32_t?
  for (int i = 5; i < std::numeric_limits<uint8_t>::max(); ++i) {
    EXPECT_EQ(IDLE, ConvertQuicPriorityToRequestPriority(i));
  }
}

TEST_P(QuicHttpUtilsTest, GetIetfVersionNumber) {
  uint32_t version = 0;
  char* version_str = reinterpret_cast<char*>(&version);
  // IETF version number are the host byte order version of the network byte
  // order on-the-wire values.
  version_str[0] = '9';
  version_str[1] = '3';
  version_str[2] = '0';
  version_str[3] = 'Q';
  EXPECT_EQ(version, GetIetfVersionNumber(QUIC_VERSION_39));
}

TEST_P(QuicHttpUtilsTest, FilterSupportedAltSvcVersionsSingleVersionGoogle) {
  // Test when a single version is supported and that single version is
  // advertised.
  for (QuicVersion version : AllSupportedVersions()) {
    QuicVersionVector supported_versions = {version};
    EXPECT_EQ(supported_versions, FilterSupportedAltSvcVersions(
                                      "quic", {version}, supported_versions));
  }
}

TEST_P(QuicHttpUtilsTest, FilterSupportedAltSvcVersionsSinglVersionIetf) {
  // Test when a single version is supported and that single version is
  // advertised.
  for (QuicVersion version : AllSupportedVersions()) {
    QuicVersionVector supported_versions = {version};
    EXPECT_EQ(supported_versions,
              FilterSupportedAltSvcVersions(
                  "hq", {GetIetfVersionNumber(version)}, supported_versions));
  }
}

TEST_P(QuicHttpUtilsTest,
       FilterSupportedAltSvcVersionsSingleVersionAdvertisedGoogle) {
  // Test when multiple versions are supported and a single version is
  // advertised.
  for (QuicVersion version : AllSupportedVersions()) {
    QuicVersionVector supported_versions = {version};
    EXPECT_EQ(supported_versions,
              FilterSupportedAltSvcVersions("quic", {version},
                                            AllSupportedVersions()));
  }
}

TEST_P(QuicHttpUtilsTest,
       FilterSupportedAltSvcVersionsSingleVersionAdvertisedIetf) {
  // Test when multiple versions are supported and a single version is
  // advertised.
  for (QuicVersion version : AllSupportedVersions()) {
    QuicVersionVector supported_versions = {version};
    EXPECT_EQ(supported_versions, FilterSupportedAltSvcVersions(
                                      "hq", {GetIetfVersionNumber(version)},
                                      AllSupportedVersions()));
  }
}

TEST_P(QuicHttpUtilsTest,
       FilterSupportedAltSvcVersionsSingleVersionSupportedGoogle) {
  // Test when a single version is supported and multiple version are
  // advertised.
  for (QuicVersion version : AllSupportedVersions()) {
    QuicVersionVector supported_versions = {version};
    std::vector<uint32_t> advertised_versions;
    for (QuicVersion version : AllSupportedVersions()) {
      advertised_versions.push_back(version);
    }
    EXPECT_EQ(supported_versions,
              FilterSupportedAltSvcVersions("quic", advertised_versions,
                                            supported_versions));
  }
}

TEST_P(QuicHttpUtilsTest,
       FilterSupportedAltSvcVersionsSingleVersionSupportedIetf) {
  // Test when a single version is supported and multiple version are
  // advertised.
  for (QuicVersion version : AllSupportedVersions()) {
    QuicVersionVector supported_versions = {version};
    std::vector<uint32_t> advertised_versions;
    for (QuicVersion version : AllSupportedVersions()) {
      advertised_versions.push_back(GetIetfVersionNumber(version));
    }
    EXPECT_EQ(supported_versions,
              FilterSupportedAltSvcVersions("hq", advertised_versions,
                                            supported_versions));
  }
}

TEST_P(QuicHttpUtilsTest, FilterSupportedAltSvcVersionsGoogle) {
  // Test when a multiple version are supported and multiple version are
  // advertised, but the sets are not identical
  QuicVersionVector supported_versions = {QUIC_VERSION_37, QUIC_VERSION_38,
                                          QUIC_VERSION_39, QUIC_VERSION_41};

  std::vector<uint32_t> advertised_versions = {QUIC_VERSION_38, QUIC_VERSION_41,
                                               QUIC_VERSION_42};

  QuicVersionVector supported_advertised_versions = {QUIC_VERSION_38,
                                                     QUIC_VERSION_41};

  EXPECT_EQ(supported_advertised_versions,
            FilterSupportedAltSvcVersions("quic", advertised_versions,
                                          supported_versions));
}

TEST_P(QuicHttpUtilsTest, FilterSupportedAltSvcVersionsIetf) {
  // Test when a multiple version are supported and multiple version are
  // advertised, but the sets are not identical
  QuicVersionVector supported_versions = {QUIC_VERSION_37, QUIC_VERSION_38,
                                          QUIC_VERSION_39, QUIC_VERSION_41};

  std::vector<uint32_t> advertised_versions = {
      GetIetfVersionNumber(QUIC_VERSION_38),
      GetIetfVersionNumber(QUIC_VERSION_41),
      GetIetfVersionNumber(QUIC_VERSION_42)};

  QuicVersionVector supported_advertised_versions = {QUIC_VERSION_38,
                                                     QUIC_VERSION_41};

  EXPECT_EQ(supported_advertised_versions,
            FilterSupportedAltSvcVersions("hq", advertised_versions,
                                          supported_versions));
}

}  // namespace test
}  // namespace net
