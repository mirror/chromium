// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef GOOGLE_APIS_GAIA_OAUTH2_ID_TOKEN_DECODER_H_
#define GOOGLE_APIS_GAIA_OAUTH2_ID_TOKEN_DECODER_H_

#include <string>
#include <vector>

namespace base {
  class Value;
}

// A class that decodes the id token received for OAuth2 token endpoint,
// and obtain the information on whether the user is a Unicorn account.
class OAuth2IdTokenDecoder {
 public:
    OAuth2IdTokenDecoder();
    ~OAuth2IdTokenDecoder();

    static bool IsChildAccount(const std::string id_token);

  private:
    static const char kChildAccountServiceFlag[];
    static const char kServicesKey[];

    static bool DecodeIdToken(const std::string id_token,
                       std::unique_ptr<base::Value> *decoded_payload);
    static bool GetServiceFlags(const std::string id_token,
                         std::vector<std::string> *service_flags);
};

#endif  // GOOGLE_APIS_GAIA_OAUTH2_ID_TOKEN_DECODER_H_
