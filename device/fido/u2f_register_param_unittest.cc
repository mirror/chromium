// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/fido/u2f_register_param.h"

#include <utility>

#include "device/fido/ctap_constants.h"
#include "device/fido/ctap_make_credential_request_param.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace device {

// Leveraging example 6 of section 7.1 of the spec
// https://fidoalliance.org/specs/fido-v2.0-rd-20170927/fido-client-to-authenticator-protocol-v2.0-rd-20170927.html
TEST(CTAPU2fRegisterParamTest, TestConvertMakeCredentialRequestToRegister) {
  constexpr uint8_t kClientDataHash[] = {
      0x68, 0x71, 0x34, 0x96, 0x82, 0x22, 0xec, 0x17, 0x20, 0x2e, 0x42,
      0x50, 0x5f, 0x8e, 0xd2, 0xb1, 0x6a, 0xe2, 0x2f, 0x16, 0xbb, 0x05,
      0xb8, 0x8c, 0x25, 0xdb, 0x9e, 0x60, 0x26, 0x45, 0xf1, 0x41};
  constexpr uint8_t kU2fApplicationParameter[] = {
      0x11, 0x94, 0x22, 0x8D, 0xA8, 0xFD, 0xBD, 0xEE, 0xFD, 0x26, 0x1B,
      0xD7, 0xB6, 0x59, 0x5C, 0xFD, 0x70, 0xA5, 0x0D, 0x70, 0xC6, 0x40,
      0x7B, 0xCF, 0x01, 0x3D, 0xE9, 0x6D, 0x4E, 0xFB, 0x17, 0xDE};

  PublicKeyCredentialRPEntity rp("acme.com");
  rp.SetRPName("Acme");

  PublicKeyCredentialUserEntity user(
      std::vector<uint8_t>{0x10, 0x98, 0x23, 0x72, 0x35, 0x40, 0x98, 0x72});

  user.SetUserName("johnpsmith@example.com")
      .SetDisplayName("John P. Smith")
      .SetIconUrl(GURL("https://pics.acme.com/00/p/aBjjjpqPb.png"));

  CtapMakeCredentialRequestParam make_credential_param(
      std::vector<uint8_t>(kClientDataHash, std::end(kClientDataHash)),
      std::move(rp), std::move(user),
      PublicKeyCredentialParams({{"public-key", -7}, {"public-key", -7}}));

  EXPECT_TRUE(make_credential_param.CheckU2fInteropCriteria());
  EXPECT_THAT(make_credential_param.GetU2fApplicationParameter(),
              testing::ElementsAreArray(kU2fApplicationParameter));
  EXPECT_THAT(make_credential_param.GetU2fChallengeParameter(),
              testing::ElementsAreArray(kClientDataHash));
}

TEST(ApduTest, TestCreateU2fRegister) {
  constexpr bool kNoIndividualAttestation = false;
  constexpr bool kIndividualAttestation = true;
  std::vector<uint8_t> app_id(kAppIdDigestLen, 0x01);
  std::vector<uint8_t> challenge_parameter(kChallengeDigestLen, 0xff);

  U2fRegisterParam register_param(app_id, challenge_parameter,
                                  kNoIndividualAttestation);
  auto encoded_register_param = register_param.Encode();
  ASSERT_TRUE(encoded_register_param);
  ASSERT_LE(2u, encoded_register_param->size());

  // Individual attestation bit should be cleared.
  EXPECT_EQ(0, (*encoded_register_param)[2] & 0x80);
  auto decoded_apdu_command =
      apdu::ApduCommand::CreateFromMessageForTesting(*encoded_register_param);
  ASSERT_TRUE(decoded_apdu_command);
  EXPECT_THAT(decoded_apdu_command->GetEncodedCommand(),
              testing::ContainerEq(*encoded_register_param));

  register_param =
      U2fRegisterParam(app_id, challenge_parameter, kIndividualAttestation);
  encoded_register_param = register_param.Encode();
  ASSERT_TRUE(encoded_register_param);
  ASSERT_LE(2u, encoded_register_param->size());

  // Individual attestation bit should be cleared.
  EXPECT_EQ(0, (*encoded_register_param)[2] & 0x80);
  decoded_apdu_command =
      apdu::ApduCommand::CreateFromMessageForTesting(*encoded_register_param);

  ASSERT_TRUE(decoded_apdu_command);
  EXPECT_THAT(decoded_apdu_command->GetEncodedCommand(),
              testing::ContainerEq(*encoded_register_param));

  // Expect null result with incorrectly sized appid.
  app_id.push_back(0xff);
  EXPECT_EQ(nullptr, U2fRegisterParam(app_id, challenge_parameter,
                                      kNoIndividualAttestation)
                         .Encode());

  // Expect null result with incorrectly sized challenge.
  app_id.pop_back();
  challenge_parameter.push_back(0xff);
  EXPECT_EQ(nullptr, U2fRegisterParam(app_id, challenge_parameter,
                                      kNoIndividualAttestation)
                         .Encode());
}

}  // namespace device
