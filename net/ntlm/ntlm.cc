// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/ntlm/ntlm.h"

#include <string.h>

// TODO(zentaro): The net package doesn't like this include.
#include "base/i18n/case_conversion.h"
#include "base/logging.h"
#include "base/md5.h"
#include "net/ntlm/des.h"
#include "net/ntlm/md4.h"
#include "net/ntlm/ntlm_buffer_writer.h"
#include "third_party/boringssl/src/include/openssl/hmac.h"

namespace net {
namespace ntlm {

namespace {
void UpdateTargetInfoAvPairs(bool is_mic_enabled,
                             bool is_epa_enabled,
                             const std::string& spn,
                             // TODO: channel bindings here?
                             std::vector<AvPair>* av_pairs,
                             uint64_t* server_timestamp,
                             size_t* target_info_len) {
  // Do a pass to update flags and calculate current length and
  // pull out the server timestamp if it is there.
  *server_timestamp = UINT64_MAX;
  *target_info_len = 0;

  bool need_flags_added = is_mic_enabled;
  for (AvPair& pair : *av_pairs) {
    *target_info_len += pair.avlen + kAvPairHeaderLen;
    if (pair.avid == TargetInfoAvId::FLAGS) {
      if (is_mic_enabled) {
        pair.flags = pair.flags | TargetInfoAvFlags::MIC_PRESENT;
      }

      need_flags_added = false;
    } else if (pair.avid == TargetInfoAvId::TIMESTAMP) {
      *server_timestamp = pair.timestamp;
    }
  }

  if (need_flags_added) {
    DCHECK(is_mic_enabled);
    AvPair flags_pair;
    flags_pair.avid = TargetInfoAvId::FLAGS;
    flags_pair.avlen = sizeof(uint32_t);
    flags_pair.flags = TargetInfoAvFlags::MIC_PRESENT;

    av_pairs->push_back(flags_pair);
    *target_info_len += kAvPairHeaderLen + flags_pair.avlen;
  }

  if (is_epa_enabled) {
    AvPair channel_binding_pair;
    channel_binding_pair.avid = TargetInfoAvId::CHANNEL_BINDINGS;
    channel_binding_pair.avlen = kChannelBindingsHashLen;
    // TODO: Hash here?
    // channel_binding_pair.ptr = nullptr;

    av_pairs->push_back(channel_binding_pair);
    *target_info_len += kAvPairHeaderLen + channel_binding_pair.avlen;

    AvPair spn_pair;
    spn_pair.avid = TargetInfoAvId::TARGET_NAME;
    spn_pair.avlen = spn.length() * 2;
    // TODO: Spn here?
    // spn_pair.ptr = nullptr;

    av_pairs->push_back(spn_pair);
    *target_info_len += kAvPairHeaderLen + spn_pair.avlen;
  }

  // Add extra for the terminator at the end.
  *target_info_len += kAvPairHeaderLen;
}

void WriteUpdatedTargetInfo(const std::string& channel_bindings,
                            const std::string& spn,
                            const std::vector<AvPair>& av_pairs,
                            size_t updated_target_info_len,
                            std::unique_ptr<uint8_t[]>* updated_target_info) {
  base::MD5Digest channel_bindings_hash;

  bool result = true;
  NtlmBufferWriter writer(updated_target_info_len);
  for (const AvPair& pair : av_pairs) {
    result = writer.WriteAvPairHeader(pair.avid, pair.avlen);
    DCHECK(result);

    switch (pair.avid) {
      case TargetInfoAvId::FLAGS:
        DCHECK(pair.avlen == sizeof(uint32_t));
        result = writer.WriteUInt32(static_cast<uint32_t>(pair.flags));
        break;
      case TargetInfoAvId::CHANNEL_BINDINGS:
        DCHECK(pair.avlen == kChannelBindingsHashLen);
        GenerateChannelBindingHashV2(channel_bindings, &channel_bindings_hash);
        result =
            writer.WriteBytes(channel_bindings_hash.a, kChannelBindingsHashLen);
        break;
      case TargetInfoAvId::TARGET_NAME:
        DCHECK(pair.avlen == spn.length() * 2);
        result = writer.WriteUtf8AsUtf16String(spn);
        break;
      default:
        result = writer.WriteBytes(pair.buffer.data(), pair.avlen);
        break;
    }

    DCHECK(result);
  }

  result = writer.WriteAvPairTerminator();
  DCHECK(result);
  DCHECK(writer.IsEndOfBuffer());
  updated_target_info->reset(new uint8_t[writer.GetLength()]);
  memcpy(updated_target_info->get(), writer.GetBuffer().data(),
         writer.GetBuffer().size());
}

}  // namespace

void GenerateNtlmHashV1(const base::string16& password, uint8_t* hash) {
  size_t length = password.length() * 2;
  NtlmBufferWriter writer(length);

  // The writer will handle the big endian case if necessary.
  bool result = writer.WriteUtf16String(password);
  DCHECK(result);

  weak_crypto::MD4Sum(
      reinterpret_cast<const uint8_t*>(writer.GetBuffer().data()), length,
      hash);
}

void GenerateResponseDesl(const uint8_t* hash,
                          const uint8_t* challenge,
                          uint8_t* response) {
  // See DESL(K, D) function in [MS-NLMP] Section 6
  uint8_t key1[8];
  uint8_t key2[8];
  uint8_t key3[8];

  // The last 2 bytes of the hash are zero padded (5 zeros) as the
  // input to generate key3.
  uint8_t padded_hash[7];
  padded_hash[0] = hash[14];
  padded_hash[1] = hash[15];
  memset(padded_hash + 2, 0, 5);

  DESMakeKey(hash, key1);
  DESMakeKey(hash + 7, key2);
  DESMakeKey(padded_hash, key3);

  DESEncrypt(key1, challenge, response);
  DESEncrypt(key2, challenge, response + 8);
  DESEncrypt(key3, challenge, response + 16);
}

void GenerateNtlmResponseV1(const base::string16& password,
                            const uint8_t* challenge,
                            uint8_t* ntlm_response) {
  uint8_t ntlm_hash[kNtlmHashLen];
  GenerateNtlmHashV1(password, ntlm_hash);
  GenerateResponseDesl(ntlm_hash, challenge, ntlm_response);
}

void GenerateResponsesV1(const base::string16& password,
                         const uint8_t* server_challenge,
                         uint8_t* lm_response,
                         uint8_t* ntlm_response) {
  GenerateNtlmResponseV1(password, server_challenge, ntlm_response);

  // In NTLM v1 (with LMv1 disabled), the lm_response and ntlm_response are the
  // same. So just copy the ntlm_response into the lm_response.
  memcpy(lm_response, ntlm_response, kResponseLenV1);
}

void GenerateLMResponseV1WithSessionSecurity(const uint8_t* client_challenge,
                                             uint8_t* lm_response) {
  // In NTLM v1 with Session Security (aka NTLM2) the lm_response is 8 bytes of
  // client challenge and 16 bytes of zeros. (See 3.3.1)
  memcpy(lm_response, client_challenge, kChallengeLen);
  memset(lm_response + kChallengeLen, 0, kResponseLenV1 - kChallengeLen);
}

void GenerateSessionHashV1WithSessionSecurity(const uint8_t* server_challenge,
                                              const uint8_t* client_challenge,
                                              base::MD5Digest* session_hash) {
  base::MD5Context ctx;
  base::MD5Init(&ctx);
  base::MD5Update(
      &ctx, base::StringPiece(reinterpret_cast<const char*>(server_challenge),
                              kChallengeLen));
  base::MD5Update(
      &ctx, base::StringPiece(reinterpret_cast<const char*>(client_challenge),
                              kChallengeLen));

  base::MD5Final(session_hash, &ctx);
}

void GenerateNtlmResponseV1WithSessionSecurity(const base::string16& password,
                                               const uint8_t* server_challenge,
                                               const uint8_t* client_challenge,
                                               uint8_t* ntlm_response) {
  // Generate the NTLMv1 Hash.
  uint8_t ntlm_hash[kNtlmHashLen];
  GenerateNtlmHashV1(password, ntlm_hash);

  // Generate the NTLMv1 Session Hash.
  base::MD5Digest session_hash;
  GenerateSessionHashV1WithSessionSecurity(server_challenge, client_challenge,
                                           &session_hash);

  // Only the first 8 bytes of |session_hash.a| are actually used.
  GenerateResponseDesl(ntlm_hash, session_hash.a, ntlm_response);
}

void GenerateResponsesV1WithSessionSecurity(const base::string16& password,
                                            const uint8_t* server_challenge,
                                            const uint8_t* client_challenge,
                                            uint8_t* lm_response,
                                            uint8_t* ntlm_response) {
  GenerateLMResponseV1WithSessionSecurity(client_challenge, lm_response);
  GenerateNtlmResponseV1WithSessionSecurity(password, server_challenge,
                                            client_challenge, ntlm_response);
}

void GenerateNtlmHashV2(const base::string16& domain,
                        const base::string16& username,
                        const base::string16& password,
                        uint8_t* v2_hash) {
  // NOTE: According to [MS-NLMP] Section 3.3.2 only the username and not the
  // domain is uppercased.
  base::string16 upper_username = base::i18n::ToUpper(username);

  uint8_t v1_hash[kNtlmHashLen];
  GenerateNtlmHashV1(password, v1_hash);

  HMAC_CTX ctx;
  HMAC_CTX_init(&ctx);
  HMAC_Init_ex(&ctx, v1_hash, kNtlmHashLen, EVP_md5(), NULL);
  DCHECK(HMAC_size(&ctx) == kNtlmHashLen);
  HMAC_Update(&ctx, reinterpret_cast<const uint8_t*>(upper_username.data()),
              upper_username.length() * 2);
  HMAC_Update(&ctx, reinterpret_cast<const uint8_t*>(domain.data()),
              domain.length() * 2);
  HMAC_Final(&ctx, v2_hash, nullptr);
  HMAC_CTX_cleanup(&ctx);
}

void GenerateProofInputV2(uint64_t timestamp,
                          const uint8_t* client_challenge,
                          std::unique_ptr<uint8_t[]>* proof_input) {
  NtlmBufferWriter writer(kProofInputLenV2);
  bool result = writer.WriteUInt16(PROOF_INPUT_VERSION) &&
                writer.WriteZeros(6) && writer.WriteUInt64(timestamp) &&
                writer.WriteBytes(client_challenge, kChallengeLen) &&
                writer.WriteZeros(4) && writer.IsEndOfBuffer();

  DCHECK(result);
  proof_input->reset(new uint8_t[kProofInputLenV2]);
  memcpy(proof_input->get(), writer.GetBuffer().data(),
         writer.GetBuffer().size());
}

void GenerateNtlmProofV2(const uint8_t* v2_hash,
                         const uint8_t* v2_input,
                         const uint8_t* server_challenge,
                         const uint8_t* target_info,
                         size_t target_info_len,
                         uint8_t* v2_proof) {
  // XXX: Input is defined at 2.2.2.7 NTLM v2: NTLMv2_CLIENT_CHALLENGE
  HMAC_CTX ctx;
  HMAC_CTX_init(&ctx);
  HMAC_Init_ex(&ctx, v2_hash, kNtlmHashLen, EVP_md5(), NULL);
  DCHECK(HMAC_size(&ctx) == kNtlmProofLenV2);
  HMAC_Update(&ctx, server_challenge, kChallengeLen);
  HMAC_Update(&ctx, v2_input, kProofInputLenV2);
  HMAC_Update(&ctx, target_info, target_info_len);
  const uint32_t zero = 0;
  HMAC_Update(&ctx, reinterpret_cast<const uint8_t*>(&zero), sizeof(uint32_t));
  HMAC_Final(&ctx, v2_proof, nullptr);
  HMAC_CTX_cleanup(&ctx);
}

void GenerateSessionBaseKeyV2(const uint8_t* v2_hash,
                              const uint8_t* v2_proof,
                              uint8_t* session_key) {
  HMAC_CTX ctx;
  HMAC_CTX_init(&ctx);
  HMAC_Init_ex(&ctx, v2_hash, kNtlmHashLen, EVP_md5(), NULL);
  DCHECK(HMAC_size(&ctx) == kSessionKeyLenV2);
  HMAC_Update(&ctx, v2_proof, kNtlmProofLenV2);
  HMAC_Final(&ctx, session_key, nullptr);
  HMAC_CTX_cleanup(&ctx);
}

void GenerateChannelBindingHashV2(const std::string& channel_bindings,
                                  base::MD5Digest* channel_bindings_hash) {
  NtlmBufferWriter writer(kEpaUnhashedStructHeaderLen);
  bool result = writer.WriteZeros(16) &&
                writer.WriteUInt32(channel_bindings.length()) &&
                writer.IsEndOfBuffer();
  DCHECK(result);

  base::MD5Context ctx;
  base::MD5Init(&ctx);
  base::MD5Update(&ctx, base::StringPiece(reinterpret_cast<const char*>(
                                              writer.GetBuffer().data()),
                                          writer.GetBuffer().size()));
  base::MD5Update(&ctx, channel_bindings);
  base::MD5Final(channel_bindings_hash, &ctx);
}

void GenerateMicV2(const uint8_t* session_key,
                   const Buffer& negotiate_msg,
                   const Buffer& challenge_msg,
                   const Buffer& authenticate_msg,
                   uint8_t* mic) {
  HMAC_CTX ctx;
  HMAC_CTX_init(&ctx);
  HMAC_Init_ex(&ctx, session_key, kNtlmHashLen, EVP_md5(), NULL);
  DCHECK(HMAC_size(&ctx) == kMicLenV2);
  HMAC_Update(&ctx, negotiate_msg.data(), negotiate_msg.size());
  HMAC_Update(&ctx, challenge_msg.data(), challenge_msg.size());
  HMAC_Update(&ctx, authenticate_msg.data(), authenticate_msg.size());
  HMAC_Final(&ctx, mic, nullptr);
  HMAC_CTX_cleanup(&ctx);
}

NET_EXPORT_PRIVATE void GenerateUpdatedTargetInfo(
    bool is_mic_enabled,
    bool is_epa_enabled,
    const std::string& channel_bindings,
    const std::string& spn,
    const std::vector<AvPair>& av_pairs,
    uint64_t* server_timestamp,
    std::unique_ptr<uint8_t[]>* updated_target_info,
    size_t* updated_target_info_len) {
  // TODO(zentaro): Refactor the two called functions better.
  std::vector<AvPair> updated_av_pairs(av_pairs);
  UpdateTargetInfoAvPairs(is_mic_enabled, is_epa_enabled, spn,
                          &updated_av_pairs, server_timestamp,
                          updated_target_info_len);
  WriteUpdatedTargetInfo(channel_bindings, spn, updated_av_pairs,
                         *updated_target_info_len, updated_target_info);
}

}  // namespace ntlm
}  // namespace net
