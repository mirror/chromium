// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/policy/user_network_configuration_updater.h"

#include <utility>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/logging.h"
#include "base/values.h"
#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/chromeos/login/session/user_session_manager.h"
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

void OnCertificatesImported(bool /* unused success */) {}

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

void UserNetworkConfigurationUpdater::SetCertificateImporterForTest(
    std::unique_ptr<chromeos::onc::CertificateImporter> certificate_importer) {
  SetCertificateImporter(std::move(certificate_importer));
}

void UserNetworkConfigurationUpdater::AddPolicyProvidedCertsObserver(
    PolicyProvidedCertsObserver* observer) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  observer_list_.AddObserver(observer);
}

void UserNetworkConfigurationUpdater::RemovePolicyProvidedCertsObserver(
    PolicyProvidedCertsObserver* observer) {
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
      certs_(std::make_unique<chromeos::onc::OncParsedCertificates>()),
      weak_factory_(this) {}

// static
void UserNetworkConfigurationUpdater::SetCertficateImporterFactoryForTest(
    chromeos::onc::CertificateImporter::Factory* certificate_importer_factory) {
  g_certificate_importer_factory_for_test = certificate_importer_factory;
}

void UserNetworkConfigurationUpdater::GetAllServerAndAuthorityCertificates(
    net::CertificateList* all_server_and_authority_certs) const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  all_server_and_authority_certs->clear();

  for (const chromeos::onc::OncParsedCertificates::ServerOrAuthorityCertificate&
           server_or_authority_cert :
       certs_->server_or_authority_certificates()) {
    all_server_and_authority_certs->push_back(
        server_or_authority_cert.certificate());
  }
}

void UserNetworkConfigurationUpdater::GetWebTrustedCertificates(
    net::CertificateList* trust_anchors) const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  trust_anchors->clear();

  for (const chromeos::onc::OncParsedCertificates::ServerOrAuthorityCertificate&
           server_or_authority_cert :
       certs_->server_or_authority_certificates()) {
    if (allow_trusted_certificates_from_policy_ &&
        server_or_authority_cert.web_trust_requested()) {
      trust_anchors->push_back(server_or_authority_cert.certificate());
    }
  }
}

void UserNetworkConfigurationUpdater::ImportCertificates(
    const base::ListValue& certificates_onc) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  std::unique_ptr<chromeos::onc::OncParsedCertificates> certs =
      std::make_unique<chromeos::onc::OncParsedCertificates>(certificates_onc);

  bool server_or_authority_certs_changed =
      !certs_->HasSameServerOrAuthorityCertificates(certs.get());
  bool client_certs_changed = !certs_->HasSameClientCertificates(certs.get());

  if (!server_or_authority_certs_changed && !client_certs_changed)
    return;

  certs_ = std::move(certs);

  if (server_or_authority_certs_changed)
    NotifyPolicyProvidedCertsChanged();

  // If certificate importer is not yet set, the import of client certificates
  // will be re-triggered in SetCertificateImporter.
  if (certificate_importer_) {
    certificate_importer_->ImportClientCertificates(
        *certs_, onc_source_, base::BindOnce(OnCertificatesImported));
  }
}

void UserNetworkConfigurationUpdater::ApplyNetworkPolicy(
    base::ListValue* network_configs_onc,
    base::DictionaryValue* global_network_config) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(user_);
  chromeos::onc::ExpandStringPlaceholdersInNetworksForUser(user_,
                                                           network_configs_onc);

  // Call on UserSessionManager to send the user's password to session manager
  // if the password substitution variable exists in the ONC.
  bool send_password =
      chromeos::onc::HasUserPasswordSubsitutionVariable(network_configs_onc);
  chromeos::UserSessionManager::GetInstance()->OnUserNetworkPolicyParsed(
      send_password);

  network_config_handler_->SetPolicy(onc_source_,
                                     user_->username_hash(),
                                     *network_configs_onc,
                                     *global_network_config);
}

void UserNetworkConfigurationUpdater::Observe(
    int type,
    const content::NotificationSource& source,
    const content::NotificationDetails& details) {
  DCHECK_EQ(type, chrome::NOTIFICATION_PROFILE_ADDED);
  Profile* profile = content::Source<Profile>(source).ptr();

  GetNSSCertDatabaseForProfile(
      profile,
      base::Bind(
          &UserNetworkConfigurationUpdater::CreateAndSetCertificateImporter,
          weak_factory_.GetWeakPtr()));
}

chromeos::onc::CertificateImporter::Factory*
UserNetworkConfigurationUpdater::GetCertificateImporterFactory() {
  if (g_certificate_importer_factory_for_test)
    return g_certificate_importer_factory_for_test;

  return certificate_importer_factory_.get();
}

void UserNetworkConfigurationUpdater::CreateAndSetCertificateImporter(
    net::NSSCertDatabase* database) {
  DCHECK(database);
  SetCertificateImporter(
      GetCertificateImporterFactory()->CreateCertificateImporter(database));
}

void UserNetworkConfigurationUpdater::SetCertificateImporter(
    std::unique_ptr<chromeos::onc::CertificateImporter> certificate_importer) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  bool first_certificate_importer_set = certificate_importer_ == nullptr;
  certificate_importer_ = std::move(certificate_importer);

  if (first_certificate_importer_set &&
      !certs_->client_certificates().empty()) {
    certificate_importer_->ImportClientCertificates(
        *certs_, onc_source_, base::BindOnce(OnCertificatesImported));
  }
}

void UserNetworkConfigurationUpdater::NotifyPolicyProvidedCertsChanged() {
  net::CertificateList all_server_and_authority_certs;
  net::CertificateList trust_anchors;
  GetAllServerAndAuthorityCertificates(&all_server_and_authority_certs);
  GetWebTrustedCertificates(&trust_anchors);

  for (auto& observer : observer_list_)
    observer.OnPolicyProvidedCertsChanged(all_server_and_authority_certs,
                                          trust_anchors);
}

}  // namespace policy
