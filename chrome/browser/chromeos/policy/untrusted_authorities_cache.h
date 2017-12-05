// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_POLICY_UNTRUSTED_AUTHORITIES_CACHE_H__
#define CHROME_BROWSER_CHROMEOS_POLICY_UNTRUSTED_AUTHORITIES_CACHE_H__

#include <string>
#include <vector>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "chromeos/login/login_state.h"
#include "net/cert/scoped_nss_types.h"

namespace base {
class SequencedTaskRunner;
}

namespace policy {

// Holds untrusted intermediate authority certificates in memory as
// ScopedCERTCertificates, making them available for client certificate
// discovery.
// This is intended for SAML flows on the sign-in screen, so we only hold
// certificates while no user has logged in.
class UntrustedAuthoritiesCache : public chromeos::LoginState::Observer {
 public:
  // All NSS operations will be performed on |io_task_runner|.
  UntrustedAuthoritiesCache(
      const scoped_refptr<base::SequencedTaskRunner>& io_task_runner);
  ~UntrustedAuthoritiesCache() override;

  // Updates the list of untrusted authority certificates from policy.
  void UpdateUntrustedAuthoritiesFromPolicy(
      const std::vector<std::string>& x509_authority_certs);

  // LoginState::Observer
  void LoggedInStateChanged() override;

 private:
  void ClearUntrustedAuthorities();

  // Parses |x509_authrotiy_certs| and updates |untrusted_authorities_|.
  void UpdateUntrustedAuthoritiesOnIOThread(
      const std::vector<std::string>& x509_authority_certs);

  bool user_logged_in_ = false;

  // The task runner to use for NSS operations.
  scoped_refptr<base::SequencedTaskRunner> io_task_runner_;

  // The actual cache of untrusted authorities. Accessed from the IO thread
  // only.
  net::ScopedCERTCertificateList untrusted_authorities_;

  base::WeakPtrFactory<UntrustedAuthoritiesCache> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(UntrustedAuthoritiesCache);
};

}  // namespace policy

#endif  // CHROME_BROWSER_CHROMEOS_POLICY_UNTRUSTED_AUTHORITIES_CACHE_H__
