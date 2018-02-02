// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_LOADER_SIGNED_EXCHANGE_SIGNATURE_VERIFIER_H_
#define CONTENT_BROWSER_LOADER_SIGNED_EXCHANGE_SIGNATURE_VERIFIER_H_

#include <string>

#include "base/memory/ref_counted.h"
#include "base/optional.h"
#include "content/common/content_export.h"
#include "net/http/http_response_headers.h"

namespace content {

class CONTENT_EXPORT SignedExchangeSignatureVerifier final {
 public:
  struct Input {
   public:
    Input();
    ~Input();

    std::string method;
    std::string url;

    std::vector<std::string> signed_header_names;
    scoped_refptr<net::HttpResponseHeaders> response_headers;
  };

  static bool Verify(const Input& input);

  // Exposed for testing.
  static base::Optional<std::vector<uint8_t>> EncodeCanonicalExchangeHeaders(
      const Input& input);
};

}  // namespace content

#endif  // CONTENT_BROWSER_LOADER_SIGNED_EXCHANGE_SIGNATURE_VERIFIER_H_
