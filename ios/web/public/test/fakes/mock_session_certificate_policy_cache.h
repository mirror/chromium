// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Due to the use of GMOCK, this header file cannot be included from .h files.
// Include directly from the .mm file instead.

#ifndef IOS_WEB_PUBLIC_TEST_FAKES_MOCK_SESSION_CERTIFICATE_POLICY_CACHE_H_
#define IOS_WEB_PUBLIC_TEST_FAKES_MOCK_SESSION_CERTIFICATE_POLICY_CACHE_H_

#include "ios/web/public/web_state/session_certificate_policy_cache.h"

#include "net/cert/x509_certificate.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace web {

class CertificatePolicyCache;

// GMock implementation of SessionCertificatePolicyCache used by TestWebState.
class MockSessionCertificatePolicyCache : public SessionCertificatePolicyCache {
 public:
  MockSessionCertificatePolicyCache();
  ~MockSessionCertificatePolicyCache() override;

  MOCK_METHOD3(RegisterAllowedCertificate,
               void(const scoped_refptr<net::X509Certificate>,
                    const std::string&,
                    net::CertStatus));
  MOCK_METHOD0(ClearAllowedCertificates, void());
  MOCK_CONST_METHOD1(UpdateCertificatePolicyCache,
                     void(const scoped_refptr<CertificatePolicyCache>&));
};

MockSessionCertificatePolicyCache::MockSessionCertificatePolicyCache() {}

MockSessionCertificatePolicyCache::~MockSessionCertificatePolicyCache() {}

}  // namespace web

#endif  // IOS_WEB_PUBLIC_TEST_FAKES_MOCK_SESSION_CERTIFICATE_POLICY_CACHE_H_
