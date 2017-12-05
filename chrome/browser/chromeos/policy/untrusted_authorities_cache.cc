// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/policy/untrusted_authorities_cache.h"

#include "base/bind.h"
#include "base/location.h"
#include "base/sequenced_task_runner.h"
#include "chromeos/network/onc/onc_utils.h"

namespace policy {

UntrustedAuthoritiesCache::UntrustedAuthoritiesCache(
    const scoped_refptr<base::SequencedTaskRunner>& io_task_runner)
    : io_task_runner_(io_task_runner), weak_ptr_factory_(this) {
  if (chromeos::LoginState::IsInitialized())
    chromeos::LoginState::Get()->AddObserver(this);
}

UntrustedAuthoritiesCache::~UntrustedAuthoritiesCache() {
  if (chromeos::LoginState::IsInitialized())
    chromeos::LoginState::Get()->RemoveObserver(this);
}

void UntrustedAuthoritiesCache::UpdateUntrustedAuthoritiesFromPolicy(
    const std::vector<std::string>& x509_authority_certs) {
  if (user_logged_in_)
    return;

  io_task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(
          &UntrustedAuthoritiesCache::UpdateUntrustedAuthoritiesOnIOThread,
          weak_ptr_factory_.GetWeakPtr(), std::move(x509_authority_certs)));
}

void UntrustedAuthoritiesCache::ClearUntrustedAuthorities() {
  std::vector<std::string> no_authority_certs;
  io_task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(
          &UntrustedAuthoritiesCache::UpdateUntrustedAuthoritiesOnIOThread,
          weak_ptr_factory_.GetWeakPtr(), std::move(no_authority_certs)));
}

void UntrustedAuthoritiesCache::UpdateUntrustedAuthoritiesOnIOThread(
    const std::vector<std::string>& x509_authority_certs) {
  net::ScopedCERTCertificateList untrusted_authorities_new;

  for (const auto& x509_authority_cert : x509_authority_certs) {
    net::ScopedCERTCertificate x509_cert =
        chromeos::onc::DecodePEMCertificate(x509_authority_cert);
    if (!x509_cert.get()) {
      LOG(ERROR) << "Unable to create untrusted authority certificate from PEM "
                    "encoding";
      continue;
    }

    untrusted_authorities_new.push_back(std::move(x509_cert));
  }
  untrusted_authorities_.swap(untrusted_authorities_new);
}

void UntrustedAuthoritiesCache::LoggedInStateChanged() {
  user_logged_in_ = chromeos::LoginState::Get()->IsUserLoggedIn();

  if (user_logged_in_)
    ClearUntrustedAuthorities();
}

}  // namespace policy
