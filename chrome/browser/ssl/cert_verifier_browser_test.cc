// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ssl/cert_verifier_browser_test.h"

#include "chrome/browser/profiles/profile_io_data.h"

CertVerifierBrowserTest::~CertVerifierBrowserTest() {}

void CertVerifierBrowserTest::SetUpInProcessBrowserTestFixture() {
  ProfileIOData::SetCertVerifierFactoryForTesting(&mock_cert_verifier_factory_);
}

void CertVerifierBrowserTest::TearDownInProcessBrowserTestFixture() {
  ProfileIOData::SetCertVerifierFactoryForTesting(nullptr);
}

net::MockCertVerifier* CertVerifierBrowserTest::mock_cert_verifier() {
  return mock_cert_verifier_factory_.mock_cert_verifier();
}
