// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/proximity_auth/webui/pollux_webui_handler.h"

#include "base/base64url.h"
#include "base/bind.h"
#include "components/proximity_auth/logging/logging.h"
#include "components/proximity_auth/pollux/credential_utils.h"
#include "crypto/random.h"

namespace pollux {

namespace {

std::string Base64Encode(const std::string& data) {
  std::string encoded_value;
  base::Base64UrlEncode(data, base::Base64UrlEncodePolicy::INCLUDE_PADDING,
                        &encoded_value);

  return encoded_value;
}

std::string Base64Decode(const std::string& encoded_data) {
  std::string decoded_value;
  if (!base::Base64UrlDecode(encoded_data,
                             base::Base64UrlDecodePolicy::REQUIRE_PADDING,
                             &decoded_value)) {
    PA_LOG(ERROR) << "Error deconding from base64";
  }

  return decoded_value;
}

}  // namespace

PolluxWebUIHandler::PolluxWebUIHandler() : weak_ptr_factory_(this) {}

PolluxWebUIHandler::~PolluxWebUIHandler() {}

void PolluxWebUIHandler::RegisterMessages() {
  web_ui()->RegisterMessageCallback(
      "createMasterKey",
      base::Bind(&PolluxWebUIHandler::CreateMasterKey, base::Unretained(this)));

  web_ui()->RegisterMessageCallback(
      "deriveCloudPairingKeys",
      base::Bind(&PolluxWebUIHandler::DeriveCloudPairingKeys,
                 base::Unretained(this)));

  web_ui()->RegisterMessageCallback(
      "createNewChallenge", base::Bind(&PolluxWebUIHandler::CreateNewChallenge,
                                       base::Unretained(this)));

  web_ui()->RegisterMessageCallback(
      "getAssertion",
      base::Bind(&PolluxWebUIHandler::GetAssertion, base::Unretained(this)));

  web_ui()->RegisterMessageCallback(
      "stopAssertion",
      base::Bind(&PolluxWebUIHandler::StopAssertion, base::Unretained(this)));
}

void PolluxWebUIHandler::CreateMasterKey(const base::ListValue* args) {
  PA_LOG(INFO) << "CreateMasterKey()";
  std::string master_key = credential_utils::GenerateMasterCloudPairingKey();
  web_ui()->CallJavascriptFunctionUnsafe("PolluxInterface.onMasterKeyCreated",
                                         base::Value(Base64Encode(master_key)));
}

void PolluxWebUIHandler::DeriveCloudPairingKeys(const base::ListValue* args) {
  std::string master_key_b64;
  if (args->GetSize() != 1 || !args->GetString(0, &master_key_b64)) {
    PA_LOG(ERROR) << "Invalid arguments to deriveCloudPairingKeys()";
    return;
  }

  PA_LOG(INFO) << "DeriveCloudPairingKeys(): " << master_key_b64;

  credential_utils::CloudPairingKeys keys =
      credential_utils::DeriveCloudPairingKeys(Base64Decode(master_key_b64));

  web_ui()->CallJavascriptFunctionUnsafe(
      "PolluxInterface.onCloudPairingKeysDerived",
      base::Value(Base64Encode(keys.irk)), base::Value(Base64Encode(keys.lk)));
}

void PolluxWebUIHandler::CreateNewChallenge(const base::ListValue* args) {
  std::string irk_b64;
  std::string lk_b64;
  if (args->GetSize() != 2 || !args->GetString(0, &irk_b64) ||
      !args->GetString(1, &lk_b64)) {
    PA_LOG(ERROR) << "Invalid arguments to createNewChallenge()";
    return;
  }

  PA_LOG(INFO) << "CreateNewChallenge(): " << irk_b64 << ", " << lk_b64;

  credential_utils::CloudPairingKeys cloud_keys;
  cloud_keys.irk = Base64Decode(irk_b64);
  cloud_keys.lk = Base64Decode(lk_b64);

  char nonce_bytes[4];
  crypto::RandBytes(nonce_bytes, 4);
  std::string nonce(nonce_bytes, 4);

  std::string key_handle = "key_handle";
  std::unique_ptr<AssertionSession::Parameters> parameters =
      credential_utils::GenerateChallenge(key_handle, cloud_keys, nonce);
  if (!parameters)
    return;

  web_ui()->CallJavascriptFunctionUnsafe(
      "PolluxInterface.onChallengeCreated", base::Value(Base64Encode(nonce)),
      base::Value(Base64Encode(parameters->challenge)),
      base::Value(Base64Encode(parameters->eid)),
      base::Value(Base64Encode(parameters->session_key)));
}

void PolluxWebUIHandler::GetAssertion(const base::ListValue* args) {
  std::string challenge_b64;
  std::string eid_b64;
  std::string sk_b64;
  if (args->GetSize() != 3 || !args->GetString(0, &challenge_b64) ||
      !args->GetString(1, &eid_b64) || !args->GetString(2, &sk_b64)) {
    PA_LOG(ERROR) << "Invalid arguments to getAssertion()";
    return;
  }

  PA_LOG(INFO) << "GetAssertion()";

  AssertionSession::Parameters parameters;
  parameters.challenge = Base64Decode(challenge_b64);
  parameters.eid = Base64Decode(eid_b64);
  parameters.session_key = Base64Decode(sk_b64);
  parameters.timeout = base::TimeDelta::FromMinutes(5);
  assertion_session_.reset(new AssertionSession(parameters));
  assertion_session_->AddObserver(this);
  assertion_session_->Start();
}

void PolluxWebUIHandler::StopAssertion(const base::ListValue* args) {
  PA_LOG(INFO) << "StopAssertion()";
  assertion_session_.reset();
  web_ui()->CallJavascriptFunctionUnsafe(
      "PolluxInterface.onAssertionStateChanged", base::Value(0));
}

void PolluxWebUIHandler::OnStateChanged(AssertionSession::State old_state,
                                        AssertionSession::State new_state) {
  PA_LOG(INFO) << "OnStateChanged: " << static_cast<int>(new_state);
  web_ui()->CallJavascriptFunctionUnsafe(
      "PolluxInterface.onAssertionStateChanged",
      base::Value(static_cast<int>(new_state)));
}

void PolluxWebUIHandler::OnGotAssertion(const std::string& assertion) {
  PA_LOG(INFO) << "OnGotAssertion: " << assertion;
}

void PolluxWebUIHandler::OnError(const std::string& error_message) {
  PA_LOG(INFO) << "OnError: " << error_message;
}

}  // namespace pollux
