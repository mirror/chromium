// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_POLICY_USER_NETWORK_CONFIGURATION_UPDATER_H_
#define CHROME_BROWSER_CHROMEOS_POLICY_USER_NETWORK_CONFIGURATION_UPDATER_H_

#include <memory>
#include <vector>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "base/sequence_checker.h"
#include "chrome/browser/chromeos/policy/network_configuration_updater.h"
#include "chromeos/network/onc/onc_certificate_importer.h"
#include "chromeos/network/onc/onc_parsed_certificates.h"
#include "components/keyed_service/core/keyed_service.h"
#include "content/public/browser/notification_observer.h"
#include "content/public/browser/notification_registrar.h"
#include "net/cert/scoped_nss_types.h"

class Profile;

namespace base {
class ListValue;
}

namespace user_manager {
class User;
}

namespace net {
class NSSCertDatabase;
class X509Certificate;
typedef std::vector<scoped_refptr<X509Certificate> > CertificateList;
}

namespace policy {

class PolicyService;

// Implements additional special handling of ONC user policies. Namely string
// expansion with the user's name (or email address, etc.) and handling of "Web"
// trust of certificates.
class UserNetworkConfigurationUpdater : public NetworkConfigurationUpdater,
                                        public KeyedService,
                                        public content::NotificationObserver {
 public:
  class PolicyProvidedCertsObserver {
   public:
    // Is called everytime the list of policy-set server and authority
    // certificates changes.
    virtual void OnPolicyProvidedCertsChanged(
        const net::CertificateList& all_server_and_authority_certs,
        const net::CertificateList& trust_anchors) = 0;
  };

  ~UserNetworkConfigurationUpdater() override;

  // Creates an updater that applies the ONC user policy from |policy_service|
  // for user |user| once the policy service is completely initialized and on
  // each policy change. Imported certificates, that request it, are only
  // granted Web trust if |allow_trusted_certs_from_policy| is true. A reference
  // to |user| is stored. It must outlive the returned updater.
  static std::unique_ptr<UserNetworkConfigurationUpdater> CreateForUserPolicy(
      Profile* profile,
      bool allow_trusted_certs_from_policy,
      const user_manager::User& user,
      PolicyService* policy_service,
      chromeos::ManagedNetworkConfigurationHandler* network_config_handler);

  void AddPolicyProvidedCertsObserver(PolicyProvidedCertsObserver* observer);
  void RemovePolicyProvidedCertsObserver(PolicyProvidedCertsObserver* observer);

  // Fills |all_sever_and_authority_certs| with all server and authority
  // certificates successfully parsed from ONC, independet of their trust bits.
  void GetAllServerAndAuthorityCertificates(
      net::CertificateList* all_server_and_authority_certs) const;

  // Fills |trust_anchors| with the server and authority certificates which were
  // successfully parsed from ONC and were granted web trust. This means that
  // the certificates had the "Web" trust bit set, and this
  // UserNetworkConfigurationUpdater instance was created with
  // |allow_trusted_certs_from_policy| = true.
  void GetWebTrustedCertificates(net::CertificateList* trust_anchors) const;

  // Helper method to expose |SetCertificateImporter| for usage in tests.
  void SetCertificateImporterForTest(
      std::unique_ptr<chromeos::onc::CertificateImporter> certificate_importer);

  static void SetCertficateImporterFactoryForTest(
      chromeos::onc::CertificateImporter::Factory*
          certificate_importer_factory);

 private:
  class CrosTrustAnchorProvider;

  UserNetworkConfigurationUpdater(
      Profile* profile,
      bool allow_trusted_certs_from_policy,
      const user_manager::User& user,
      PolicyService* policy_service,
      chromeos::ManagedNetworkConfigurationHandler* network_config_handler);

  // NetworkConfigurationUpdater:
  void ImportCertificates(const base::ListValue& certificates_onc) override;
  void ApplyNetworkPolicy(
      base::ListValue* network_configs_onc,
      base::DictionaryValue* global_network_config) override;

  // content::NotificationObserver implementation. Observes the profile to which
  // |this| belongs to for PROFILE_ADDED notification.
  void Observe(int type,
               const content::NotificationSource& source,
               const content::NotificationDetails& details) override;

  // Creates onc::CertImporter with |database| and passes it to
  // |SetCertificateImporter|.
  void CreateAndSetCertificateImporter(net::NSSCertDatabase* database);

  // Sets the certificate importer that should be used to import certificate
  // policies. If there is |pending_certificates_onc_|, it gets imported.
  void SetCertificateImporter(
      std::unique_ptr<chromeos::onc::CertificateImporter> certificate_importer);

  void NotifyPolicyProvidedCertsChanged();

  chromeos::onc::CertificateImporter::Factory* GetCertificateImporterFactory();

  std::unique_ptr<chromeos::onc::CertificateImporter::Factory>
      certificate_importer_factory_;

  // Whether Web trust is allowed or not.
  bool allow_trusted_certificates_from_policy_;

  // The user for whom the user policy will be applied.
  const user_manager::User* user_;

  base::ObserverList<PolicyProvidedCertsObserver, true> observer_list_;

  // Holds certificates from the last parsed ONC policy.
  std::unique_ptr<chromeos::onc::OncParsedCertificates> certs_;

  // Certificate importer to be used for importing policy defined certificates.
  // Set by |SetCertificateImporter|.
  std::unique_ptr<chromeos::onc::CertificateImporter> certificate_importer_;

  SEQUENCE_CHECKER(sequence_checker_);

  base::WeakPtrFactory<UserNetworkConfigurationUpdater> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(UserNetworkConfigurationUpdater);
};

}  // namespace policy

#endif  // CHROME_BROWSER_CHROMEOS_POLICY_USER_NETWORK_CONFIGURATION_UPDATER_H_
