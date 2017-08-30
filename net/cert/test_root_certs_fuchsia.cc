// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/cert/test_root_certs.h"

#include "base/location.h"
#include "base/logging.h"
#include "net/cert/internal/cert_errors.h"
#include "net/cert/internal/parsed_certificate.h"
#include "net/cert/internal/system_trust_store.h"
#include "net/cert/x509_certificate.h"
#include "net/cert/x509_util.h"

namespace net {

bool TestRootCerts::Add(X509Certificate* certificate) {
  std::string der_encoded;
  if (!X509Certificate::GetDEREncoded(certificate->os_cert_handle(),
                                      &der_encoded)) {
    LOG(FATAL) << "GetDEREncoded() failed.";
  }
  CertErrors err;
  auto parsed =
      ParsedCertificate::Create(net::x509_util::CreateCryptoBuffer(der_encoded),
                                ParseCertificateOptions(), &err);
  CHECK(parsed) << err.ToDebugString();
  temporary_roots_.push_back(parsed);
  return true;
}

void TestRootCerts::Clear() {
  temporary_roots_.clear();
}

bool TestRootCerts::IsEmpty() const {
  return temporary_roots_.empty();
}

TestRootCerts::~TestRootCerts() {}

void TestRootCerts::Init() {}

}  // namespace net
