// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_TOOLS_CERT_MAPPER_ENTRY_H_
#define NET_TOOLS_CERT_MAPPER_ENTRY_H_

#include <vector>

#include "net/der/input.h"

namespace net {

class Entry {
 public:
  enum class Type {
    // Corresponds with X509_entry (i.e. NOT a precert)
    kLeafCert,

    // This is a fabricated type to simplify iterating over all the extra certs
    // in addition to the leaf certs.
    kExtraCert,
  };

  Entry();
  explicit Entry(const der::Input& cert);
  Entry(Type type, const der::Input& cert);
  Entry(const Entry&);
  ~Entry();

  Type type = Type::kLeafCert;

  std::vector<der::Input> certs;
};

}  // namespace net

#endif  // NET_TOOLS_CERT_MAPPER_ENTRY_H_
