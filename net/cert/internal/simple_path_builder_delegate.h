// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_CERT_INTERNAL_SIMPLE_PATH_BUILDER_DELEGATE_H_
#define NET_CERT_INTERNAL_SIMPLE_PATH_BUILDER_DELEGATE_H_

#include <stddef.h>

#include "base/compiler_specific.h"
#include "net/base/net_export.h"
#include "net/cert/internal/path_builder.h"

namespace net {

class CertErrors;
class SignatureAlgorithm;

// SimpleSignaturePolicy modifies the base SignaturePolicy by allowing the
// minimum RSA key length to be specified (rather than hard coded to 2048).
class NET_EXPORT SimplePathBuilderDelegate : public CertPathBuilderDelegate {
 public:
  explicit SimplePathBuilderDelegate(size_t min_rsa_modulus_length_bits);

  bool VerifyCertificateSignature(const SignatureAlgorithm& signature_algorithm,
                                  const der::Input& tbs_certificate_tlv,
                                  const der::BitString& signature_value,
                                  const der::Input& public_key_spki,
                                  CertErrors* errors) const override;

  void CheckPathAfterVerification(const CertPath& path,
                                  CertPathErrors* errors) const override;

 private:
  const size_t min_rsa_modulus_length_bits_;
};

// TODO(crbug.com/634443): Move exported errors to a central location?
extern CertErrorId kRsaModulusTooSmall;

}  // namespace net

#endif  // NET_CERT_INTERNAL_SIMPLE_PATH_BUILDER_DELEGATE_H_
