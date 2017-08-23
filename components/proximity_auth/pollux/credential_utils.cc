// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/proximity_auth/pollux/credential_utils.h"

#include <iomanip>

#include "base/memory/ptr_util.h"
#include "components/proximity_auth/logging/logging.h"
#include "crypto/hkdf.h"
#include "crypto/hmac.h"
#include "crypto/random.h"

namespace pollux {
namespace credential_utils {

namespace {
const size_t kKeySize = 32;
const char kIrkPurpose[] = "irk";
const char kLkPurpose[] = "lk";
const uint8_t kSalt[] = {0x82, 0xAA, 0x55, 0xA0, 0xD3, 0x97, 0xF8, 0x83,
                         0x46, 0xCA, 0x1C, 0xEE, 0x8D, 0x39, 0x09, 0xB9,
                         0x5F, 0x13, 0xFA, 0x7D, 0xEB, 0x1D, 0x4A, 0xB3,
                         0x83, 0x76, 0xB8, 0x25, 0x6D, 0xA8, 0x55, 0x10};

std::string ToHex(uint8_t* bytes, size_t size) {
	std::stringstream ss;
  ss << "0x";
	for (size_t i = 0; i < size; ++i) {
		uint8_t byte = bytes[i];
    ss << std::hex << std::setfill('0') << std::setw(2)
       << static_cast<uint64_t>(byte);
  }
  return ss.str();
}

}  // namespace

std::string GenerateMasterCloudPairingKey() {
  char key_bytes[kKeySize];
  crypto::RandBytes(key_bytes, kKeySize);
  return std::string(key_bytes, kKeySize);
}

CloudPairingKeys DeriveCloudPairingKeys(const std::string& master_key) {
  std::string salt(reinterpret_cast<const char*>(&kSalt[0]), sizeof(kSalt));
  crypto::HKDF irk(master_key, salt, kIrkPurpose, kKeySize, 0, 0);
  crypto::HKDF lk(master_key, salt, kLkPurpose, kKeySize, 0, 0);

  return {irk.client_write_key().as_string(),
          lk.client_write_key().as_string()};
}

std::unique_ptr<AssertionSession::Parameters> GenerateChallenge(
    const std::string& authenticator_key_handle,
    const CloudPairingKeys& cloud_pairing_keys,
    const std::string& nonce) {
  if (nonce.size() != 4) {
    PA_LOG(ERROR) << "Invalid nonce: " << nonce.size() << " bytes, expected 4.";
    return nullptr;
  }

  if (cloud_pairing_keys.irk.size() != kKeySize ||
      cloud_pairing_keys.lk.size() != kKeySize) {
    PA_LOG(ERROR) << "Invalid cloud pairing keys.";
    return nullptr;
  }

  //        4bytes   4 bytes
  // EID = [ nonce ] [ hash ]
  crypto::HMAC eid_hmac(crypto::HMAC::HashAlgorithm::SHA256);
  uint8_t hash[kKeySize];
  if (!eid_hmac.Init(cloud_pairing_keys.irk) ||
      !eid_hmac.Sign(nonce, &hash[0], kKeySize)) {
    PA_LOG(ERROR) << "Error creating EID";
    return nullptr;
  }
  PA_LOG(WARNING) << "HMAC: " << ToHex(hash, kKeySize);
  std::string eid = nonce + std::string(reinterpret_cast<char*>(&hash[0]), 4);

  // SK = HMAC(nonce, LK);
  crypto::HMAC sk_hmac(crypto::HMAC::HashAlgorithm::SHA256);
  uint8_t sk_bytes[kKeySize];
  if (!eid_hmac.Init(cloud_pairing_keys.lk) ||
      !eid_hmac.Sign(nonce, &sk_bytes[0], kKeySize)) {
    PA_LOG(ERROR) << "Error creating SK";
    return nullptr;
  }
  std::string session_key(reinterpret_cast<char*>(&sk_bytes[0]), kKeySize);

  std::unique_ptr<AssertionSession::Parameters> parameters =
      base::MakeUnique<AssertionSession::Parameters>();
  parameters->eid = eid;
  parameters->session_key = session_key;
  parameters->challenge = "challenge";
  parameters->timeout = base::TimeDelta::FromMinutes(5);
  return parameters;
}

std::string DeriveRemoteEid(const std::string& session_key,
                            const std::string& local_eid) {
  crypto::HMAC hmac(crypto::HMAC::HashAlgorithm::SHA256);
  uint8_t hash[kKeySize];
  if (!hmac.Init(session_key) || !hmac.Sign(local_eid, &hash[0], kKeySize)) {
    PA_LOG(ERROR) << "Error deriving remote EID";
    return std::string();
  }
  return std::string(reinterpret_cast<char*>(&hash[0]), 8);
}

}  // namespace credential_utils
}  // namespace pollux
