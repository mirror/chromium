// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_CERT_CERT_VERIFIER_FACTORY_H_
#define NET_CERT_CERT_VERIFIER_FACTORY_H_

#include <memory>

namespace net {

class CertVerifier;

class CertVerifierFactory {
 public:
  virtual ~CertVerifierFactory() = default;

  virtual std::unique_ptr<CertVerifier> CreateCertVerifier() = 0;
};

}  // namespace net

#endif  // NET_CERT_CERT_VERIFIER_FACTORY_H_
