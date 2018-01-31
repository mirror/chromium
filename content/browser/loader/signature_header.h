// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_LOADER_SIGNATURE_HEADER_H_
#define CONTENT_BROWSER_LOADER_SIGNATURE_HEADER_H_

#include <stdint.h>
#include <string>
#include <vector>
#include "base/macros.h"
#include "content/common/content_export.h"

namespace content {

// Parser for the Signature header.
class CONTENT_EXPORT SignatureHeader {
 public:
  struct Signature {
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
  explicit SignatureHeader(const std::string& signature_str);
  ~SignatureHeader();

  // Returns parsed signature list, or empty if parse error.
  const std::vector<Signature>& Signatures() const { return signatures_; }

 private:
  std::vector<Signature> signatures_;
  DISALLOW_COPY_AND_ASSIGN(SignatureHeader);
};

// Parses the value of Signed-Headers header.
bool CONTENT_EXPORT ParseSignedHeaders(const std::string& signed_headers_str,
                                       std::vector<std::string>* headers);

}  // namespace content

#endif  // CONTENT_BROWSER_LOADER_SIGNATURE_HEADER_H_
