// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_TOOLS_CERT_MAPPER_ENTRY_H_
#define NET_TOOLS_CERT_MAPPER_ENTRY_H_

#include <vector>

#include "base/memory/ref_counted.h"
#include "net/der/input.h"

namespace net {

class CertBytes : public base::RefCountedThreadSafe<CertBytes> {
 public:
  // Make a copy of the data.
  CertBytes(const void* data, size_t length);

  std::string AsString() const;
  base::StringPiece AsStringPiece() const;
  der::Input AsInput() const;

 private:
  std::vector<uint8_t> bytes_;
  ~CertBytes();
  friend class base::RefCountedThreadSafe<CertBytes>;
};

using CertBytesVector = std::vector<scoped_refptr<CertBytes>>;

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
  Entry(const Entry&);
  Entry(Entry&&);
  ~Entry();

  Type type = Type::kLeafCert;

  CertBytesVector certs;
};

}  // namespace net

#endif  // NET_TOOLS_CERT_MAPPER_ENTRY_H_
