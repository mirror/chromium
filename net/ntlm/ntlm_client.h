// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Based on [MS-NLMP]: NT LAN Manager (NTLM) Authentication Protocol
// Specification version 28.0 [1]. Additional NTLM reference [2].
//
// [1] https://msdn.microsoft.com/en-us/library/cc236621.aspx
// [2] http://davenport.sourceforge.net/ntlm.html

#ifndef NET_BASE_NTLM_CLIENT_H_
#define NET_BASE_NTLM_CLIENT_H_

#include <stddef.h>
#include <stdint.h>

#include <memory>
#include <string>

#include "base/strings/string16.h"
#include "base/strings/string_piece.h"
#include "net/base/net_export.h"
#include "net/ntlm/ntlm_constants.h"

namespace net {
namespace ntlm {

// Provides an implementation of an NTLMv2 Client with support
// for MIC and EPA.
class NET_EXPORT_PRIVATE NtlmClient {
 public:
  // Pass the version of the protocol to implement, and the default features
  // for that version will be used.
  //
  // For NTLMv1 this is with Extended Security Enabled (aka NTLM2).
  // For NTLMv2 Extended Protection for Authentication (EPA) and Message
  // Integrity Check (MIC) are enabled by default.
  explicit NtlmClient(ntlm::NtlmVersion version);

  // For finer level control of the protocol features.
  // |version| defines the version of responses.
  // |feature_flags| allows some specific sub-features to be disabled. If that
  // feature is not relevant in the requested |version| they are ignored.
  // |negotiate_flags| This is the flag set sent in the negotiate message.
  NtlmClient(ntlm::NtlmVersion version, ntlm::NtlmFeatures feature_flags);
  ~NtlmClient();

  bool IsNtlmV2() const { return version_ == ntlm::NtlmVersion::kNtlmV2; }

  bool IsMicEnabled() const {
    return IsNtlmV2() && ((feature_flags_ & ntlm::NtlmFeatures::DISABLE_MIC) !=
                          ntlm::NtlmFeatures::DISABLE_MIC);
  }

  bool IsEpaEnabled() const {
    return IsNtlmV2() && ((feature_flags_ & ntlm::NtlmFeatures::DISABLE_EPA) !=
                          ntlm::NtlmFeatures::DISABLE_EPA);
  }

  // Returns a |Buffer| containing the Negotiate message.
  Buffer GetNegotiateMessage() const;

  // Returns a |Buffer| containing the Authenticate message. If the method
  // fails an empty |Buffer| is returned.
  //
  // |hostname| can be a short NetBIOS name or an FQDN, however the server will
  // only inspect this field if the default domain policy is to restrict NTLM.
  // In this case the hostname will be compared to a whitelist stored in this
  // group policy [1].
  // |client_challenge| must contain 8 bytes of random data.
  // |server_challenge_message| is the full content of the challenge message
  // sent by the server.
  //
  // [1] - https://technet.microsoft.com/en-us/library/jj852267(v=ws.11).aspx
  Buffer GenerateAuthenticateMessage(
      const base::string16& domain,
      const base::string16& username,
      const base::string16& password,
      const std::string& hostname,
      const std::string& channel_bindings,
      const std::string& spn,
      uint64_t client_time,
      const uint8_t* client_challenge,
      const Buffer& server_challenge_message) const;

  Buffer GenerateAuthenticateMessageV1(
      const base::string16& domain,
      const base::string16& username,
      const base::string16& password,
      const std::string& hostname,
      const uint8_t* client_challenge,
      const Buffer& server_challenge_message) const {
    if (version_ != ntlm::NtlmVersion::kNtlmV1)
      return Buffer();

    return GenerateAuthenticateMessage(
        domain, username, password, hostname, std::string(), std::string(), 0,
        client_challenge, server_challenge_message);
  }

 private:
  // Returns the length of the Authenticate message based on the length of the
  // variable length parts of the message and whether Unicode support was
  // negotiated.
  size_t CalculateAuthenticateMessageLength(
      bool is_unicode,
      const base::string16& domain,
      const base::string16& username,
      const std::string& hostname,
      size_t updated_target_info_len) const;

  void CalculatePayloadLayout(bool is_unicode,
                              const base::string16& domain,
                              const base::string16& username,
                              const std::string& hostname,
                              size_t updated_target_info_len,
                              SecurityBuffer* lm_info,
                              SecurityBuffer* ntlm_info,
                              SecurityBuffer* domain_info,
                              SecurityBuffer* username_info,
                              SecurityBuffer* hostname_info,
                              size_t* authenticate_message_len) const;

  // Returns the length of the header part of the Authenticate message.
  // NOTE: When NTLMv2 support is added this is no longer a fixed value.
  size_t GetAuthenticateHeaderLength() const;

  // Returns the length of the NTLM response.
  // NOTE: When NTLMv2 support is added this is no longer a fixed value.
  size_t GetNtlmResponseLength(size_t updated_target_info_len) const;

  // Generates the negotiate message (which is always the same) into
  // |negotiate_message_|.
  void GenerateNegotiateMessage();

  NtlmVersion version_;
  NtlmFeatures feature_flags_;
  NegotiateFlags negotiate_flags_;
  Buffer negotiate_message_;

  DISALLOW_COPY_AND_ASSIGN(NtlmClient);
};

}  // namespace ntlm
}  // namespace net

#endif  // NET_BASE_NTLM_CLIENT_H_