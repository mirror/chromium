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

enum class EntryType {
  // The 0th certificate is the target certificate (likely end-entity), and
  // the rest are intermediates. The intermediates may or may not be correctly
  // ordered (probably will be). The trust anchor is probably not included in
  // the chain.
  kTlsCertificateChain,

  // Corresponds with X509_entry (i.e. NOT a precert) from the CT database.
  // The 0th certificate is the end-entity, and all subsequent certificates
  // are intermediates (in order) up to and including the trusted root
  // certificate.
  kCTCertificateChain,

  // The set of unique intermediates used in the chains.
  kUniqueIntermediateCertificate,
};

class Entry {
 public:
  Entry();
  Entry(const Entry&);
  Entry(Entry&&);
  ~Entry();

  EntryType type = EntryType::kTlsCertificateChain;

  CertBytesVector certs;
};

}  // namespace net

#endif  // NET_TOOLS_CERT_MAPPER_ENTRY_H_
