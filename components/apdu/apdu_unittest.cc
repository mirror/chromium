// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/memory/ptr_util.h"
#include "components/apdu/apdu_command.h"
#include "components/apdu/apdu_response.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace apdu {

TEST(ApduTest, TestDeserializeBasic) {
  uint8_t cla = 0xAA;
  uint8_t ins = 0xAB;
  uint8_t p1 = 0xAC;
  uint8_t p2 = 0xAD;
  std::vector<uint8_t> message = {cla, ins, p1, p2};
  std::unique_ptr<APDUCommand> cmd = APDUCommand::CreateFromMessage(message);

  EXPECT_EQ(static_cast<size_t>(0), cmd->response_length_);
  EXPECT_THAT(cmd->data_, testing::ContainerEq(std::vector<uint8_t>()));
  EXPECT_EQ(cmd->cla_, cla);
  EXPECT_EQ(cmd->ins_, ins);
  EXPECT_EQ(cmd->p1_, p1);
  EXPECT_EQ(cmd->p2_, p2);

  // Invalid length
  message = {cla, ins, p1};
  EXPECT_EQ(nullptr, APDUCommand::CreateFromMessage(message));
  message.push_back(p2);
  message.push_back(0);
  message.push_back(0xFF);
  message.push_back(0xFF);
  std::vector<uint8_t> oversized(APDUCommand::kApduMaxDataLength);
  message.insert(message.end(), oversized.begin(), oversized.end());
  message.push_back(0);
  message.push_back(0);
  EXPECT_NE(nullptr, APDUCommand::CreateFromMessage(message));
  message.push_back(0);
  EXPECT_EQ(nullptr, APDUCommand::CreateFromMessage(message));
}

TEST(ApduTest, TestDeserializeComplex) {
  uint8_t cla = 0xAA;
  uint8_t ins = 0xAB;
  uint8_t p1 = 0xAC;
  uint8_t p2 = 0xAD;
  std::vector<uint8_t> data(
      APDUCommand::kApduMaxDataLength - APDUCommand::kApduMaxHeader - 2, 0x7F);
  std::vector<uint8_t> message = {cla, ins, p1, p2, 0};
  message.push_back((data.size() >> 8) & 0xff);
  message.push_back(data.size() & 0xff);
  message.insert(message.end(), data.begin(), data.end());

  // Create a message with no response expected
  std::unique_ptr<APDUCommand> cmd_no_response =
      APDUCommand::CreateFromMessage(message);
  EXPECT_EQ(static_cast<size_t>(0), cmd_no_response->response_length_);
  EXPECT_THAT(data, testing::ContainerEq(cmd_no_response->data_));
  EXPECT_EQ(cmd_no_response->cla_, cla);
  EXPECT_EQ(cmd_no_response->ins_, ins);
  EXPECT_EQ(cmd_no_response->p1_, p1);
  EXPECT_EQ(cmd_no_response->p2_, p2);

  // Add response length to message
  message.push_back(0xF1);
  message.push_back(0xD0);
  std::unique_ptr<APDUCommand> cmd = APDUCommand::CreateFromMessage(message);
  EXPECT_THAT(data, testing::ContainerEq(cmd->data_));
  EXPECT_EQ(cmd->cla_, cla);
  EXPECT_EQ(cmd->ins_, ins);
  EXPECT_EQ(cmd->p1_, p1);
  EXPECT_EQ(cmd->p2_, p2);
  EXPECT_EQ(static_cast<size_t>(0xF1D0), cmd->response_length_);
}

TEST(ApduTest, TestDeserializeResponse) {
  APDUResponse::Status status;
  std::unique_ptr<APDUResponse> response;
  std::vector<uint8_t> test_vector;

  // Invalid length
  std::vector<uint8_t> message = {0xAA};
  EXPECT_EQ(nullptr, APDUResponse::CreateFromMessage(message));

  // Valid length and status
  status = APDUResponse::Status::SW_CONDITIONS_NOT_SATISFIED;
  message = {static_cast<uint8_t>(static_cast<uint16_t>(status) >> 8),
             static_cast<uint8_t>(status)};
  response = APDUResponse::CreateFromMessage(message);
  ASSERT_NE(nullptr, response);
  EXPECT_EQ(APDUResponse::Status::SW_CONDITIONS_NOT_SATISFIED,
            response->response_status_);
  EXPECT_THAT(response->data_, testing::ContainerEq(std::vector<uint8_t>()));

  // Valid length and status
  status = APDUResponse::Status::SW_NO_ERROR;
  message = {static_cast<uint8_t>(static_cast<uint16_t>(status) >> 8),
             static_cast<uint8_t>(status)};
  test_vector = {0x01, 0x02, 0xEF, 0xFF};
  message.insert(message.begin(), test_vector.begin(), test_vector.end());
  response = APDUResponse::CreateFromMessage(message);
  ASSERT_NE(nullptr, response);
  EXPECT_EQ(APDUResponse::Status::SW_NO_ERROR, response->response_status_);
  EXPECT_THAT(response->data_, testing::ContainerEq(test_vector));
}

TEST(ApduTest, TestSerializeCommand) {
  auto cmd = std::make_unique<APDUCommand>();

  cmd->set_cla(0xA);
  cmd->set_ins(0xB);
  cmd->set_p1(0xC);
  cmd->set_p2(0xD);

  // No data, no response expected
  std::vector<uint8_t> expected({0xA, 0xB, 0xC, 0xD});
  ASSERT_THAT(expected, testing::ContainerEq(cmd->GetEncodedCommand()));
  EXPECT_THAT(
      expected,
      testing::ContainerEq(
          APDUCommand::CreateFromMessage(expected)->GetEncodedCommand()));

  // No data, response expected
  cmd->set_response_length(0xCAFE);
  expected = {0xA, 0xB, 0xC, 0xD, 0x0, 0xCA, 0xFE};
  EXPECT_THAT(expected, testing::ContainerEq(cmd->GetEncodedCommand()));
  EXPECT_THAT(
      expected,
      testing::ContainerEq(
          APDUCommand::CreateFromMessage(expected)->GetEncodedCommand()));

  // Data exists, response expected
  std::vector<uint8_t> data({0x1, 0x2, 0x3, 0x4});
  cmd->set_data(data);
  expected = {0xA, 0xB, 0xC, 0xD, 0x0,  0x0, 0x4,
              0x1, 0x2, 0x3, 0x4, 0xCA, 0xFE};
  EXPECT_THAT(expected, testing::ContainerEq(cmd->GetEncodedCommand()));
  EXPECT_THAT(
      expected,
      testing::ContainerEq(
          APDUCommand::CreateFromMessage(expected)->GetEncodedCommand()));

  // Data exists, no response expected
  cmd->set_response_length(0);
  expected = {0xA, 0xB, 0xC, 0xD, 0x0, 0x0, 0x4, 0x1, 0x2, 0x3, 0x4};
  EXPECT_THAT(expected, testing::ContainerEq(cmd->GetEncodedCommand()));
  EXPECT_THAT(
      expected,
      testing::ContainerEq(
          APDUCommand::CreateFromMessage(expected)->GetEncodedCommand()));
}

TEST(ApduTest, TestSerializeEdgeCases) {
  auto cmd = std::make_unique<APDUCommand>();

  cmd->set_cla(0xA);
  cmd->set_ins(0xB);
  cmd->set_p1(0xC);
  cmd->set_p2(0xD);

  // Set response length to maximum, which should serialize to 0x0000
  cmd->set_response_length(APDUCommand::kApduMaxResponseLength);
  std::vector<uint8_t> expected = {0xA, 0xB, 0xC, 0xD, 0x0, 0x0, 0x0};
  EXPECT_THAT(expected, testing::ContainerEq(cmd->GetEncodedCommand()));
  EXPECT_THAT(
      expected,
      testing::ContainerEq(
          APDUCommand::CreateFromMessage(expected)->GetEncodedCommand()));

  // Maximum data size
  std::vector<uint8_t> oversized(APDUCommand::kApduMaxDataLength);
  cmd->set_data(oversized);
  EXPECT_THAT(cmd->GetEncodedCommand(),
              testing::ContainerEq(
                  APDUCommand::CreateFromMessage(cmd->GetEncodedCommand())
                      ->GetEncodedCommand()));
}

TEST(ApduTest, TestCreateU2FSign) {
  std::vector<uint8_t> appid(APDUCommand::kAppIdDigestLen, 0x01);
  std::vector<uint8_t> challenge(APDUCommand::kChallengeDigestLen, 0xff);
  std::vector<uint8_t> key_handle(APDUCommand::kMaxKeyHandleLength);

  std::unique_ptr<APDUCommand> cmd =
      APDUCommand::CreateU2FSign(appid, challenge, key_handle);
  ASSERT_NE(nullptr, cmd);
  EXPECT_THAT(APDUCommand::CreateFromMessage(cmd->GetEncodedCommand())
                  ->GetEncodedCommand(),
              testing::ContainerEq(cmd->GetEncodedCommand()));
  // Expect null result with incorrectly sized key handle
  key_handle.push_back(0x0f);
  cmd = APDUCommand::CreateU2FSign(appid, challenge, key_handle);
  EXPECT_EQ(nullptr, cmd);
  key_handle.pop_back();
  // Expect null result with incorrectly sized appid
  appid.pop_back();
  cmd = APDUCommand::CreateU2FSign(appid, challenge, key_handle);
  EXPECT_EQ(nullptr, cmd);
  appid.push_back(0xff);
  // Expect null result with incorrectly sized challenge
  challenge.push_back(0x0);
  cmd = APDUCommand::CreateU2FSign(appid, challenge, key_handle);
  EXPECT_EQ(nullptr, cmd);
}

TEST(ApduTest, TestCreateU2FRegister) {
  constexpr bool kNoIndividualAttestation = false;
  constexpr bool kIndividualAttestation = true;
  std::vector<uint8_t> appid(APDUCommand::kAppIdDigestLen, 0x01);
  std::vector<uint8_t> challenge(APDUCommand::kChallengeDigestLen, 0xff);
  std::unique_ptr<APDUCommand> cmd = APDUCommand::CreateU2FRegister(
      appid, challenge, kNoIndividualAttestation);
  ASSERT_NE(nullptr, cmd);
  std::vector<uint8_t> encoded = cmd->GetEncodedCommand();
  ASSERT_LE(2u, encoded.size());
  // Individual attestation bit should be cleared.
  EXPECT_EQ(0, encoded[2] & 0x80);
  EXPECT_THAT(APDUCommand::CreateFromMessage(cmd->GetEncodedCommand())
                  ->GetEncodedCommand(),
              testing::ContainerEq(encoded));

  cmd =
      APDUCommand::CreateU2FRegister(appid, challenge, kIndividualAttestation);
  ASSERT_NE(nullptr, cmd);
  encoded = cmd->GetEncodedCommand();
  ASSERT_LE(2u, encoded.size());
  // Individual attestation bit should be set.
  EXPECT_EQ(0x80, encoded[2] & 0x80);

  // Expect null result with incorrectly sized appid
  appid.push_back(0xff);
  cmd = APDUCommand::CreateU2FRegister(appid, challenge,
                                       kNoIndividualAttestation);
  EXPECT_EQ(nullptr, cmd);
  appid.pop_back();
  // Expect null result with incorrectly sized challenge
  challenge.push_back(0xff);
  cmd = APDUCommand::CreateU2FRegister(appid, challenge,
                                       kNoIndividualAttestation);
  EXPECT_EQ(nullptr, cmd);
}

TEST(ApduTest, TestCreateU2FVersion) {
  std::unique_ptr<APDUCommand> cmd = APDUCommand::CreateU2FVersion();
  std::vector<uint8_t> expected = {
      0x0, APDUCommand::kInsU2fVersion, 0x0, 0x0, 0x0, 0x0, 0x0};

  EXPECT_THAT(expected, testing::ContainerEq(cmd->GetEncodedCommand()));
  EXPECT_THAT(APDUCommand::CreateFromMessage(cmd->GetEncodedCommand())
                  ->GetEncodedCommand(),
              testing::ContainerEq(cmd->GetEncodedCommand()));
}

TEST(ApduTest, TestCreateU2FLegacyVersion) {
  std::unique_ptr<APDUCommand> cmd = APDUCommand::CreateU2FLegacyVersion();
  // Legacy version command contains 2 extra null bytes compared to ISO 7816-4
  // format
  std::vector<uint8_t> expected = {
      0x0, APDUCommand::kInsU2fVersion, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0};

  EXPECT_THAT(expected, testing::ContainerEq(cmd->GetEncodedCommand()));
  EXPECT_THAT(APDUCommand::CreateFromMessage(cmd->GetEncodedCommand())
                  ->GetEncodedCommand(),
              testing::ContainerEq(cmd->GetEncodedCommand()));
}
}  // namespace apdu
