// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_POLICY_UNTRUSTED_AUTHORITIES_CACHE_H__
#define CHROME_BROWSER_CHROMEOS_POLICY_UNTRUSTED_AUTHORITIES_CACHE_H__

#include <string>
#include <vector>

#include "base/callback_forward.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "net/cert/scoped_nss_types.h"

namespace policy {

// Holds untrusted intermediate authority certificates in memory as
// ScopedCERTCertificates, making them available for client certificate
// discovery.
class UntrustedAuthoritiesCache {
 public:
  UntrustedAuthoritiesCache(
      const std::vector<std::string>& onc_x509_authority_certs);
  ~UntrustedAuthoritiesCache();

  static std::vector<std::string> GetUntrustedAuthoritiesFromDeviceOncPolicy();

 private:
  // The actual cache of untrusted authorities.
  net::ScopedCERTCertificateList untrusted_authorities_;

  DISALLOW_COPY_AND_ASSIGN(UntrustedAuthoritiesCache);
};

}  // namespace policy

#endif  // CHROME_BROWSER_CHROMEOS_POLICY_UNTRUSTED_AUTHORITIES_CACHE_H__
