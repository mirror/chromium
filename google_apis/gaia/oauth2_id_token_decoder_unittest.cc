// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "google_apis/gaia/oauth2_id_token_decoder.h"

#include <string>
#include <vector>

#include "testing/gtest/include/gtest/gtest.h"

namespace {

const char kIdTokenInvalidJwt[] =
    "dummy-header."
    "..."
    ".dummy-signature";
const char kIdTokenInvalidJson[] =
    "dummy-header."
    "YWJj"  // payload: abc
    ".dummy-signature";
const char kIdTokenEmptyServices[] =
    "dummy-header."
    "eyAic2VydmljZXMiOiBbXSB9"  // payload: { "services": [] }
    ".dummy-signature";
const char kIdTokenMissingServices[] =
    "dummy-header."
    "eyAiYWJjIjogIjEyMyIgfQ=="  // payload: { "abc": ""}
    ".dummy-signature";
const char kIdTokenNotChildAccount[] =
    "dummy-header."
    "eyAic2VydmljZXMiOiBbImFiYyJdIH0=" // payload: {"services": ["abc"] }
    ".dummy-signature";
const char kIdTokenChildAccount[] =
    "dummy-header."
    "eyAic2VydmljZXMiOiBbInVjYSJdIH0=" // payload: {"services": ["uca"] }
    ".dummy-signature";

class OAuth2IdTokenDecoderTest : public testing::Test {
 public:
  void SetUp() override;

 protected:
  std::unique_ptr<OAuth2IdTokenDecoder> decoder_;
  std::string id_token_;
};

void OAuth2IdTokenDecoderTest::SetUp() {
  this->id_token_.clear();
}

TEST_F(OAuth2IdTokenDecoderTest, Invalid) {
  id_token_ = kIdTokenInvalidJwt;
  bool is_child_account;
  bool result = decoder_->IsChildAccount(id_token_, is_child_account);
  EXPECT_EQ(result, false);

  id_token_ = kIdTokenInvalidJson;
  result = decoder_->IsChildAccount(id_token_, is_child_account);
  EXPECT_EQ(result, false);

  id_token_ = kIdTokenMissingServices;
  result = decoder_->IsChildAccount(id_token_, is_child_account);
  EXPECT_EQ(result, false);
}

TEST_F(OAuth2IdTokenDecoderTest, NotChild) {
  id_token_ = kIdTokenEmptyServices;
  bool is_child_account;
  bool result = decoder_->IsChildAccount(id_token_, is_child_account);
  EXPECT_EQ(result, true);
  EXPECT_EQ(is_child_account, false);

  id_token_ = kIdTokenNotChildAccount;
  result = decoder_->IsChildAccount(id_token_, is_child_account);
  EXPECT_EQ(result, true);
  EXPECT_EQ(is_child_account, false);
}

TEST_F(OAuth2IdTokenDecoderTest, Child) {
  id_token_ = kIdTokenChildAccount;
  bool is_child_account;
  bool result = decoder_->IsChildAccount(id_token_, is_child_account);
  EXPECT_EQ(result, true);
  EXPECT_EQ(is_child_account, true);
}

}  // namespace


