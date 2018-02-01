// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_LOADER_SIGNED_EXCHANGE_HEADER_PARSER_H_
#define CONTENT_BROWSER_LOADER_SIGNED_EXCHANGE_HEADER_PARSER_H_

#include <stdint.h>
#include <string>
#include <vector>
#include "base/macros.h"
#include "content/common/content_export.h"

namespace content {

class CONTENT_EXPORT SignedExchangeHeaderParser {
 public:
  struct CONTENT_EXPORT Signature {
    Signature();
    Signature(const Signature&);
    ~Signature();

    std::string label;
    std::string sig;
    std::string integrity;
    std::string certUrl;
    std::string certSha256;
    std::string ed25519Key;
    std::string validityUrl;
    uint64_t date;
    uint64_t expires;
  };

  // Parses a value of the Signed-Headers header.
  // https://wicg.github.io/webpackage/draft-yasskin-http-origin-signed-responses.html#rfc.section.3.1
  static bool ParseSignedHeaders(const std::string& signed_headers_str,
                                 std::vector<std::string>* headers);

  // Parses a value of the Signature header.
  // https://wicg.github.io/webpackage/draft-yasskin-http-origin-signed-responses.html#rfc.section.3.2
  static bool ParseSignature(const std::string& signature_str,
                             std::vector<Signature>* signatures);
};

}  // namespace content

#endif  // CONTENT_BROWSER_LOADER_SIGNED_EXCHANGE_HEADER_PARSER_H_
