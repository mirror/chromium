// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/loader/signed_exchange_header_parser.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace content {

class SignatureHeaderTest : public ::testing::Test {
 protected:
  SignatureHeaderTest() {}
};

TEST_F(SignatureHeaderTest, ParseSignedHeaders) {
  const char hdr_string[] = "\"content-type\", \"digest\"";
  std::vector<std::string> headers;
  EXPECT_TRUE(
      SignedExchangeHeaderParser::ParseSignedHeaders(hdr_string, &headers));
  ASSERT_EQ(headers.size(), 2u);
  EXPECT_EQ(headers[0], "content-type");
  EXPECT_EQ(headers[1], "digest");
}

TEST_F(SignatureHeaderTest, Test1) {
  const char hdr_string[] =
      "sig1;"
      "sig=*MEUCIQDXlI2gN3RNBlgFiuRNFpZXcDIaUpX6HIEwcZEc0cZYLAIga9DsVOMM+"
      "g5YpwEBdGW3sS+bvnmAJJiSMwhuBdqp5UY;"
      "integrity=\"mi\";"
      "validityUrl=\"https://example.com/resource.validity.1511128380\";"
      "certUrl=\"https://example.com/oldcerts\";"
      "certSha256=*W7uB969dFW3Mb5ZefPS9Tq5ZbH5iSmOILpjv2qEArmI;"
      "date=1511128380; expires=1511733180";
  const uint8_t decoded_sig[] = {
      0x30, 0x45, 0x02, 0x21, 0x00, 0xd7, 0x94, 0x8d, 0xa0, 0x37, 0x74, 0x4d,
      0x06, 0x58, 0x05, 0x8a, 0xe4, 0x4d, 0x16, 0x96, 0x57, 0x70, 0x32, 0x1a,
      0x52, 0x95, 0xfa, 0x1c, 0x81, 0x30, 0x71, 0x91, 0x1c, 0xd1, 0xc6, 0x58,
      0x2c, 0x02, 0x20, 0x6b, 0xd0, 0xec, 0x54, 0xe3, 0x0c, 0xfa, 0x0e, 0x58,
      0xa7, 0x01, 0x01, 0x74, 0x65, 0xb7, 0xb1, 0x2f, 0x9b, 0xbe, 0x79, 0x80,
      0x24, 0x98, 0x92, 0x33, 0x08, 0x6e, 0x05, 0xda, 0xa9, 0xe5, 0x46};
  const uint8_t decoded_certSha256[] = {
      0x5b, 0xbb, 0x81, 0xf7, 0xaf, 0x5d, 0x15, 0x6d, 0xcc, 0x6f, 0x96,
      0x5e, 0x7c, 0xf4, 0xbd, 0x4e, 0xae, 0x59, 0x6c, 0x7e, 0x62, 0x4a,
      0x63, 0x88, 0x2e, 0x98, 0xef, 0xda, 0xa1, 0x00, 0xae, 0x62};

  std::vector<SignedExchangeHeaderParser::Signature> signatures;
  EXPECT_TRUE(
      SignedExchangeHeaderParser::ParseSignature(hdr_string, &signatures));
  ASSERT_EQ(signatures.size(), 1u);
  EXPECT_EQ(signatures[0].label, "sig1");
  EXPECT_EQ(signatures[0].sig,
            std::string(reinterpret_cast<const char*>(decoded_sig),
                        sizeof(decoded_sig)));
  EXPECT_EQ(signatures[0].integrity, "mi");
  EXPECT_EQ(signatures[0].validityUrl,
            "https://example.com/resource.validity.1511128380");
  EXPECT_EQ(signatures[0].certUrl, "https://example.com/oldcerts");
  EXPECT_EQ(signatures[0].certSha256,
            std::string(reinterpret_cast<const char*>(decoded_certSha256),
                        sizeof(decoded_certSha256)));
  EXPECT_EQ(signatures[0].date, 1511128380ul);
  EXPECT_EQ(signatures[0].expires, 1511733180ul);
}

}  // namespace content
