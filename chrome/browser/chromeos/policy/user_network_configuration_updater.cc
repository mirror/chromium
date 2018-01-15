// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/policy/user_network_configuration_updater.h"

#include <cert.h>
#include <utility>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/logging.h"
#include "base/values.h"
#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/net/nss_context.h"
#include "chrome/browser/profiles/profile.h"
#include "chromeos/network/managed_network_configuration_handler.h"
#include "chromeos/network/onc/onc_certificate_importer_impl.h"
#include "chromeos/network/onc/onc_utils.h"
#include "components/policy/policy_constants.h"
#include "components/user_manager/user.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/notification_source.h"
#include "net/cert/x509_certificate.h"
#include "net/cert/x509_util_nss.h"

namespace policy {

namespace {

// Returns true if the two lists of certificates are equal. They are considered
// equal they contain the same certificates in the same order.
bool AreCertListEqual(const net::CertificateList& list_1,
                      const net::CertificateList& list_2) {
  if (list_1.size() != list_2.size())
    return false;

  for (size_t i = 0; i < list_1.size(); ++i) {
    if (!list_1[i]->Equals(list_2[i].get()))
      return false;
  }
  return true;
}

chromeos::onc::CertificateImporter::Factory*
    g_certificate_importer_factory_for_test = nullptr;

}  // namespace

UserNetworkConfigurationUpdater::~UserNetworkConfigurationUpdater() {}

// static
std::unique_ptr<UserNetworkConfigurationUpdater>
UserNetworkConfigurationUpdater::CreateForUserPolicy(
    Profile* profile,
    bool allow_trusted_certs_from_policy,
    const user_manager::User& user,
    PolicyService* policy_service,
    chromeos::ManagedNetworkConfigurationHandler* network_config_handler) {
  std::unique_ptr<UserNetworkConfigurationUpdater> updater(
      new UserNetworkConfigurationUpdater(
          profile, allow_trusted_certs_from_policy, user, policy_service,
          network_config_handler));
  updater->Init();
  return updater;
}

void UserNetworkConfigurationUpdater::AddTrustedCertsObserver(
    WebTrustedCertsObserver* observer) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  observer_list_.AddObserver(observer);
}

void UserNetworkConfigurationUpdater::RemoveTrustedCertsObserver(
    WebTrustedCertsObserver* observer) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  observer_list_.RemoveObserver(observer);
}

UserNetworkConfigurationUpdater::UserNetworkConfigurationUpdater(
    Profile* profile,
    bool allow_trusted_certs_from_policy,
    const user_manager::User& user,
    PolicyService* policy_service,
    chromeos::ManagedNetworkConfigurationHandler* network_config_handler)
    : NetworkConfigurationUpdater(onc::ONC_SOURCE_USER_POLICY,
                                  key::kOpenNetworkConfiguration,
                                  policy_service,
                                  network_config_handler),
      certificate_importer_factory_(
          std::make_unique<chromeos::onc::CertificateImporterImpl::Factory>(
              content::BrowserThread::GetTaskRunnerForThread(
                  content::BrowserThread::IO))),
      allow_trusted_certificates_from_policy_(allow_trusted_certs_from_policy),
      user_(&user),
      weak_factory_(this) {}

// static
void UserNetworkConfigurationUpdater::SetCertficateImporterFactoryForTest(
    chromeos::onc::CertificateImporter::Factory* certificate_importer_factory) {
  g_certificate_importer_factory_for_test = certificate_importer_factory;
}

void UserNetworkConfigurationUpdater::GetWebTrustedCertificates(
    net::CertificateList* certs) const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  *certs = web_trust_certs_;
}

void UserNetworkConfigurationUpdater::OnCertificatesImported(
    bool /* unused success */,
    net::ScopedCERTCertificateList onc_trusted_certificates) {
  net::CertificateList new_web_trust_certs;
  if (allow_trusted_certificates_from_policy_) {
    new_web_trust_certs =
        net::x509_util::CreateX509CertificateListFromCERTCertificates(
            onc_trusted_certificates);
  }

  SetTrustedCertsAndNotify(new_web_trust_certs);

  // Notify callbacks waiting for trusted certs to be available.
  std::vector<base::OnceClosure> callbacks;
  callbacks.swap(trusted_certs_imported_callbacks_);
  for (auto& callback : callbacks)
    std::move(callback).Run();
}

void UserNetworkConfigurationUpdater::ImportCertificates(
    const base::ListValue& certificates_onc) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  // If certificate importer is not yet set, cache the certificate onc. It will
  // be imported when the certificate importer gets set.
  if (!certificate_importer_) {
    pending_certificates_onc_.reset(certificates_onc.DeepCopy());
    return;
  }

  certificate_importer_->ImportCertificates(
      certificates_onc, onc_source_,
      base::Bind(&UserNetworkConfigurationUpdater::OnCertificatesImported,
                 base::Unretained(this)));
}

void UserNetworkConfigurationUpdater::ApplyNetworkPolicy(
    base::ListValue* network_configs_onc,
    base::DictionaryValue* global_network_config) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(user_);
  chromeos::onc::ExpandStringPlaceholdersInNetworksForUser(user_,
                                                           network_configs_onc);
  network_config_handler_->SetPolicy(onc_source_,
                                     user_->username_hash(),
                                     *network_configs_onc,
                                     *global_network_config);
}

void UserNetworkConfigurationUpdater::SetCertificateDatabase(
    net::NSSCertDatabase* database) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  SetCertificateImporter(
      GetCertificateImporterFactory()->CreateCertificateImporter(database));
}

void UserNetworkConfigurationUpdater::WaitForPendingTrustedCertsImported(
    base::OnceClosure done_callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  // Exit immediately if no web trust certificates are allowed, any certificates
  // have already been imported, or no ONC data is available yet.
  if (!allow_trusted_certificates_from_policy_ || !web_trust_certs_.empty() ||
      !pending_certificates_onc_) {
    std::move(done_callback).Run();
    return;
  }

  // If no policy-set trust roots have been applied yet but we allow policy-set
  // trust roots, retrieve certificates which should be trusted according to ONC
  // policy and are already available.
  // This can happen before the user's NSSDatabase is available, because it only
  // depends on the user's public slot NSSDB being opened, which happens early
  // in profile initialization. The NSSDatabase on the other hand is only
  // available as soon as the user's private slot is available, which requires
  // (potentially slow) TPM operations.
  GetCertificateImporterFactory()->GetTrustedCertificatesStatus(
      pending_certificates_onc_->Clone(), onc_source_,
      base::BindOnce(
          &UserNetworkConfigurationUpdater::GotTrustedCertificatesStatus,
          weak_factory_.GetWeakPtr(), std::move(done_callback)));
}

void UserNetworkConfigurationUpdater::GotTrustedCertificatesStatus(
    base::OnceClosure done_callback,
    net::ScopedCERTCertificateList onc_trusted_certificates,
    bool has_pending_trusted_certs) {
  SetTrustedCertsAndNotify(
      net::x509_util::CreateX509CertificateListFromCERTCertificates(
          onc_trusted_certificates));

  if (has_pending_trusted_certs)
    trusted_certs_imported_callbacks_.emplace_back(std::move(done_callback));
  else
    std::move(done_callback).Run();
}

chromeos::onc::CertificateImporter::Factory*
UserNetworkConfigurationUpdater::GetCertificateImporterFactory() {
  if (g_certificate_importer_factory_for_test)
    return g_certificate_importer_factory_for_test;

  return certificate_importer_factory_.get();
}

void UserNetworkConfigurationUpdater::SetCertificateImporter(
    std::unique_ptr<chromeos::onc::CertificateImporter> certificate_importer) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  certificate_importer_ = std::move(certificate_importer);

  if (pending_certificates_onc_)
    ImportCertificates(*pending_certificates_onc_);
  pending_certificates_onc_.reset();
}

void UserNetworkConfigurationUpdater::SetTrustedCertsAndNotify(
    const net::CertificateList& new_web_trust_certs) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  CHECK(new_web_trust_certs.empty() || allow_trusted_certificates_from_policy_);

  // Only notify if the trust anchors have actually changed. Observers tend to
  // do drastic things (such as close all pending connections) when receiving
  // the OnTrustAnchorsChanged notification.
  if (AreCertListEqual(web_trust_certs_, new_web_trust_certs))
    return;

  web_trust_certs_ = new_web_trust_certs;
  for (auto& observer : observer_list_)
    observer.OnTrustAnchorsChanged(web_trust_certs_);
}

}  // namespace policy
