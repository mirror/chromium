// Copyright (c) 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/quic/core/crypto/transport_parameters.h"

#include "net/quic/platform/api/quic_arraysize.h"
#include "net/quic/platform/api/quic_test.h"
#include "third_party/boringssl/src/include/openssl/mem.h"

namespace net {
namespace test {

class TransportParametersTest : public QuicTest {};

TEST_F(TransportParametersTest, RoundTripClient) {
  TransportParameters orig_params;
  orig_params.perspective = Perspective::IS_CLIENT;
  orig_params.initial_max_stream_data = 12;
  orig_params.initial_max_data = 34;
  orig_params.idle_timeout = 56;
  orig_params.version = 0xff000005;

  std::vector<uint8_t> serialized;
  ASSERT_TRUE(SerializeTransportParameters(orig_params, &serialized));

  TransportParameters new_params;
  ASSERT_TRUE(ParseTransportParameters(serialized.data(), serialized.size(),
                                       Perspective::IS_CLIENT, &new_params));

  EXPECT_EQ(new_params.initial_max_stream_data,
            orig_params.initial_max_stream_data);
  EXPECT_EQ(new_params.initial_max_data, orig_params.initial_max_data);
  EXPECT_EQ(new_params.idle_timeout, orig_params.idle_timeout);
  EXPECT_EQ(new_params.version, orig_params.version);
}

TEST_F(TransportParametersTest, RoundTripServer) {
  TransportParameters orig_params;
  orig_params.perspective = Perspective::IS_SERVER;
  orig_params.initial_max_stream_data = 12;
  orig_params.initial_max_data = 34;
  orig_params.idle_timeout = 56;
  orig_params.stateless_reset_token.resize(16);
  orig_params.version = 0xff000005;
  orig_params.supported_versions.push_back(0xff000005);
  orig_params.supported_versions.push_back(0xff000004);

  std::vector<uint8_t> serialized;
  ASSERT_TRUE(SerializeTransportParameters(orig_params, &serialized));

  TransportParameters new_params;
  ASSERT_TRUE(ParseTransportParameters(serialized.data(), serialized.size(),
                                       Perspective::IS_SERVER, &new_params));

  EXPECT_EQ(new_params.initial_max_stream_data,
            orig_params.initial_max_stream_data);
  EXPECT_EQ(new_params.initial_max_data, orig_params.initial_max_data);
  EXPECT_EQ(new_params.idle_timeout, orig_params.idle_timeout);
  EXPECT_EQ(new_params.stateless_reset_token,
            orig_params.stateless_reset_token);
  EXPECT_EQ(new_params.version, orig_params.version);
  ASSERT_EQ(new_params.supported_versions, orig_params.supported_versions);
}

TEST_F(TransportParametersTest, IsValid) {
  TransportParameters empty_params;
  empty_params.perspective = Perspective::IS_CLIENT;
  EXPECT_TRUE(empty_params.is_valid());

  {
    TransportParameters params = empty_params;
    params.idle_timeout = 600;
    EXPECT_TRUE(params.is_valid());
    params.idle_timeout = 601;
    EXPECT_FALSE(params.is_valid());
  }
  {
    TransportParameters params = empty_params;
    params.max_packet_size.present = true;
    params.max_packet_size.value = 0;
    EXPECT_FALSE(params.is_valid());
    params.max_packet_size.value = 1200;
    EXPECT_TRUE(params.is_valid());
    params.max_packet_size.value = 65527;
    EXPECT_TRUE(params.is_valid());
    params.max_packet_size.value = 65535;
    EXPECT_FALSE(params.is_valid());
  }
  {
    TransportParameters params = empty_params;
    params.ack_delay_exponent.present = true;
    params.ack_delay_exponent.value = 0;
    EXPECT_TRUE(params.is_valid());
    params.ack_delay_exponent.value = 20;
    EXPECT_TRUE(params.is_valid());
    params.ack_delay_exponent.value = 21;
    EXPECT_FALSE(params.is_valid());
  }
}

TEST_F(TransportParametersTest, NoServerParamsWithoutStatelessResetToken) {
  TransportParameters orig_params;
  orig_params.perspective = Perspective::IS_SERVER;
  orig_params.initial_max_stream_data = 12;
  orig_params.initial_max_data = 34;
  orig_params.idle_timeout = 56;
  orig_params.version = 0xff000005;
  orig_params.supported_versions.push_back(0xff000005);
  orig_params.supported_versions.push_back(0xff000004);

  std::vector<uint8_t> out;
  ASSERT_FALSE(SerializeTransportParameters(orig_params, &out));
}

TEST_F(TransportParametersTest, NoClientParamsWithStatelessResetToken) {
  TransportParameters orig_params;
  orig_params.perspective = Perspective::IS_CLIENT;
  orig_params.initial_max_stream_data = 12;
  orig_params.initial_max_data = 34;
  orig_params.idle_timeout = 56;
  orig_params.stateless_reset_token.resize(16);
  orig_params.version = 0xff000005;

  std::vector<uint8_t> out;
  ASSERT_FALSE(SerializeTransportParameters(orig_params, &out));
}

TEST_F(TransportParametersTest, ParseClientParams) {
  const uint8_t kClientParams[] = {
      0xff, 0x00, 0x00, 0x05,  // initial version
      0x00, 0x16,              // length parameters array that follows
      // initial_max_stream_data
      0x00, 0x00,              // parameter id
      0x00, 0x04,              // length
      0x00, 0x00, 0x00, 0x0c,  // value
      // initial_max_data
      0x00, 0x01,              // parameter id
      0x00, 0x04,              // length
      0x00, 0x00, 0x00, 0x22,  // value
      // idle_timeout
      0x00, 0x03,  // parameter id
      0x00, 0x02,  // length
      0x00, 0x38,  // value
  };

  TransportParameters out_params;
  ASSERT_TRUE(ParseTransportParameters(kClientParams,
                                       QUIC_ARRAYSIZE(kClientParams),
                                       Perspective::IS_CLIENT, &out_params));
}

TEST_F(TransportParametersTest, ParseClientParamsFailsWithStatelessResetToken) {
  TransportParameters out_params;

  // clang-format off
  const uint8_t kClientParamsWithFullToken[] = {
      0xff, 0x00, 0x00, 0x05,  // initial version
      0x00, 0x2a,  // length parameters array that follows
      // initial_max_stream_data
      0x00, 0x00,              // parameter id
      0x00, 0x04,              // length
      0x00, 0x00, 0x00, 0x0c,  // value
      // initial_max_data
      0x00, 0x01,              // parameter id
      0x00, 0x04,              // length
      0x00, 0x00, 0x00, 0x22,  // value
      // idle_timeout
      0x00, 0x03,  // parameter id
      0x00, 0x02,  // length
      0x00, 0x38,  // value
      // stateless_reset_token
      0x00, 0x06,  // parameter id
      0x00, 0x10,  // length
      0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
      0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
  };
  // clang-format on

  ASSERT_FALSE(ParseTransportParameters(
      kClientParamsWithFullToken, QUIC_ARRAYSIZE(kClientParamsWithFullToken),
      Perspective::IS_CLIENT, &out_params));

  const uint8_t kClientParamsWithEmptyToken[] = {
      0xff, 0x00, 0x00, 0x05,  // initial version
      0x00, 0x1a,              // length parameters array that follows
      // initial_max_stream_data
      0x00, 0x00,              // parameter id
      0x00, 0x04,              // length
      0x00, 0x00, 0x00, 0x0c,  // value
      // initial_max_data
      0x00, 0x01,              // parameter id
      0x00, 0x04,              // length
      0x00, 0x00, 0x00, 0x22,  // value
      // idle_timeout
      0x00, 0x03,  // parameter id
      0x00, 0x02,  // length
      0x00, 0x38,  // value
      // stateless_reset_token
      0x00, 0x06,  // parameter id
      0x00, 0x00,  // length
  };

  ASSERT_FALSE(ParseTransportParameters(
      kClientParamsWithEmptyToken, QUIC_ARRAYSIZE(kClientParamsWithEmptyToken),
      Perspective::IS_CLIENT, &out_params));
}

TEST_F(TransportParametersTest, ParseClientParametersWithInvalidParams) {
  TransportParameters out_params;

  const uint8_t kClientParamsRepeated[] = {
      0xff, 0x00, 0x00, 0x05,  // initial version
      0x00, 0x1c,              // length parameters array that follows
      // initial_max_stream_data
      0x00, 0x00,              // parameter id
      0x00, 0x04,              // length
      0x00, 0x00, 0x00, 0x0c,  // value
      // initial_max_data
      0x00, 0x01,              // parameter id
      0x00, 0x04,              // length
      0x00, 0x00, 0x00, 0x22,  // value
      // idle_timeout
      0x00, 0x03,  // parameter id
      0x00, 0x02,  // length
      0x00, 0x38,  // value
      // idle_timeout (repeat)
      0x00, 0x03,  // parameter id
      0x00, 0x02,  // length
      0x00, 0x38,  // value
  };
  ASSERT_FALSE(ParseTransportParameters(kClientParamsRepeated,
                                        QUIC_ARRAYSIZE(kClientParamsRepeated),
                                        Perspective::IS_CLIENT, &out_params));

  const uint8_t kClientParamsMissing[] = {
      0xff, 0x00, 0x00, 0x05,  // initial version
      0x00, 0x10,              // length parameters array that follows
      // initial_max_stream_data
      0x00, 0x00,              // parameter id
      0x00, 0x04,              // length
      0x00, 0x00, 0x00, 0x0c,  // value
      // initial_max_data
      0x00, 0x01,              // parameter id
      0x00, 0x04,              // length
      0x00, 0x00, 0x00, 0x22,  // value
  };
  ASSERT_FALSE(ParseTransportParameters(kClientParamsMissing,
                                        QUIC_ARRAYSIZE(kClientParamsMissing),
                                        Perspective::IS_CLIENT, &out_params));
}

TEST_F(TransportParametersTest, ParseServerParams) {
  // clang-format off
  const uint8_t kServerParams[] = {
      0xff, 0x00, 0x00, 0x05,  // negotiated_version
      0x08,  // length of supported versions array
      0xff, 0x00, 0x00, 0x05,
      0xff, 0x00, 0x00, 0x04,
      0x00, 0x2a,  // length of parameters array that follows
      // initial_max_stream_data
      0x00, 0x00,
      0x00, 0x04,
      0x00, 0x00, 0x00, 0x0c,
      // initial_max_data
      0x00, 0x01,
      0x00, 0x04,
      0x00, 0x00, 0x00, 0x22,
      // idle_timeout
      0x00, 0x03,
      0x00, 0x02,
      0x00, 0x38,
      // stateless_reset_token
      0x00, 0x06,
      0x00, 0x10,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  };
  // clang-format on

  TransportParameters out_params;
  ASSERT_TRUE(ParseTransportParameters(kServerParams,
                                       QUIC_ARRAYSIZE(kServerParams),
                                       Perspective::IS_SERVER, &out_params));
}

TEST_F(TransportParametersTest, ParseServerParamsWithoutToken) {
  // clang-format off
  const uint8_t kServerParams[] = {
      0xff, 0x00, 0x00, 0x05,  // negotiated_version
      0x08,  // length of supported versions array
      0xff, 0x00, 0x00, 0x05,
      0xff, 0x00, 0x00, 0x04,
      0x00, 0x16,  // length of parameters array that follows
      // initial_max_stream_data
      0x00, 0x00,
      0x00, 0x04,
      0x00, 0x00, 0x00, 0x0c,
      // initial_max_data
      0x00, 0x01,
      0x00, 0x04,
      0x00, 0x00, 0x00, 0x22,
      // idle_timeout
      0x00, 0x03,
      0x00, 0x02,
      0x00, 0x38,
  };
  // clang-format on

  TransportParameters out_params;
  ASSERT_FALSE(ParseTransportParameters(kServerParams,
                                        QUIC_ARRAYSIZE(kServerParams),
                                        Perspective::IS_SERVER, &out_params));
}

TEST_F(TransportParametersTest, ParseServerParametersWithInvalidParams) {
  TransportParameters out_params;

  // clang-format off
  const uint8_t kServerParamsRepeated[] = {
      0xff, 0x00, 0x00, 0x05,  // negotiated_version
      0x08,  // length of supported versions array
      0xff, 0x00, 0x00, 0x05,
      0xff, 0x00, 0x00, 0x04,
      0x00, 0x30,  // length of parameters array that follows
      // initial_max_stream_data
      0x00, 0x00,
      0x00, 0x04,
      0x00, 0x00, 0x00, 0x0c,
      // initial_max_data
      0x00, 0x01,
      0x00, 0x04,
      0x00, 0x00, 0x00, 0x22,
      // idle_timeout
      0x00, 0x03,
      0x00, 0x02,
      0x00, 0x38,
      // idle_timeout (repeat)
      0x00, 0x03,
      0x00, 0x02,
      0x00, 0x38,
      // stateless_reset_token
      0x00, 0x06,
      0x00, 0x10,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  };
  // clang-format on
  ASSERT_FALSE(ParseTransportParameters(kServerParamsRepeated,
                                        QUIC_ARRAYSIZE(kServerParamsRepeated),
                                        Perspective::IS_SERVER, &out_params));

  // clang-format off
  const uint8_t kServerParamsMissing[] = {
      0xff, 0x00, 0x00, 0x05,  // negotiated_version
      0x08,  // length of supported versions array
      0xff, 0x00, 0x00, 0x05,
      0xff, 0x00, 0x00, 0x04,
      0x00, 0x24,  // length of parameters array that follows
      // initial_max_stream_data
      0x00, 0x00,
      0x00, 0x04,
      0x00, 0x00, 0x00, 0x0c,
      // initial_max_data
      0x00, 0x01,
      0x00, 0x04,
      0x00, 0x00, 0x00, 0x22,
      // stateless_reset_token
      0x00, 0x06,
      0x00, 0x10,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  };
  // clang-format on
  ASSERT_FALSE(ParseTransportParameters(kServerParamsMissing,
                                        QUIC_ARRAYSIZE(kServerParamsMissing),
                                        Perspective::IS_SERVER, &out_params));
}

}  // namespace test
}  // namespace net
