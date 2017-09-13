// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/arc/enterprise/arc_cert_store_bridge.h"

#include <cert.h>
#include <certdb.h>
#include <pk11pub.h>

#include "base/base64.h"
#include "base/memory/singleton.h"
#include "chrome/browser/chromeos/platform_keys/key_permissions.h"
#include "chrome/browser/policy/profile_policy_connector.h"
#include "chrome/browser/policy/profile_policy_connector_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "components/arc/arc_bridge_service.h"
#include "components/arc/arc_browser_context_keyed_service_factory_base.h"
#include "components/arc/arc_service_manager.h"
#include "components/policy/policy_constants.h"
#include "net/cert/scoped_nss_types.h"

namespace arc {

namespace {

const char kPolicyAllowCorporateKeyUsage[] = "allowCorporateKeyUsage";

// Singleton factory for ArcCertStoreBridge.
class ArcCertStoreBridgeFactory
    : public internal::ArcBrowserContextKeyedServiceFactoryBase<
          ArcCertStoreBridge,
          ArcCertStoreBridgeFactory> {
 public:
  // Factory name used by ArcBrowserContextKeyedServiceFactoryBase.
  static constexpr const char* kName = "ArcCertStoreBridgeFactory";

  static ArcCertStoreBridgeFactory* GetInstance() {
    return base::Singleton<ArcCertStoreBridgeFactory>::get();
  }

 private:
  friend base::DefaultSingletonTraits<ArcCertStoreBridgeFactory>;
  ArcCertStoreBridgeFactory() = default;
  ~ArcCertStoreBridgeFactory() override = default;
};

bool IsAndroidPackageName(const std::string& name) {
  return name.find(".") != std::string::npos;
}

// Returns true if the certificate is allowed to be used by ARC.
bool IsCertificateAllowed(const std::string& alias, const PrefService* prefs) {
  net::ScopedCERTCertificate cert(
      CERT_FindCertByNickname(CERT_GetDefaultCertDB(), alias.c_str()));
  if (cert == nullptr) {
    LOG(ERROR) << "Certificate with nickname=" << alias << "does not exist.";
    return false;
  }
  const SECItem& spki_der = cert->derPublicKey;
  if (!chromeos::KeyPermissions::IsCorporateKey(
          std::string(spki_der.data, spki_der.data + spki_der.len), prefs)) {
    LOG(ERROR) << "Certificate with nickname=" << alias
               << "is not allowed to be used by ARC.";
    return false;
  }
  return true;
}

}  // namespace

// static
ArcCertStoreBridge* ArcCertStoreBridge::GetForBrowserContext(
    content::BrowserContext* context) {
  return ArcCertStoreBridgeFactory::GetForBrowserContext(context);
}

ArcCertStoreBridge::ArcCertStoreBridge(content::BrowserContext* context,
                                       ArcBridgeService* bridge_service)
    : context_(context), arc_bridge_service_(bridge_service), binding_(this) {
  VLOG(1) << "ArcCertStoreBridge::Init";

  arc_bridge_service_->cert_store()->AddObserver(this);
}

ArcCertStoreBridge::~ArcCertStoreBridge() {
  VLOG(1) << "ArcCertStoreBridge::Destroy";

  arc_bridge_service_->cert_store()->RemoveObserver(this);
}

void ArcCertStoreBridge::OnInstanceReady() {
  VLOG(1) << "ArcCertStoreBridge::OnInstanceReady";

  if (policy_service_ == nullptr) {
    auto* profile_policy_connector =
        policy::ProfilePolicyConnectorFactory::GetForBrowserContext(context_);
    // This is never null.
    policy_service_ = profile_policy_connector->policy_service();
    DCHECK(policy_service_);
  }

  policy_service_->AddObserver(policy::POLICY_DOMAIN_CHROME, this);

  auto* instance =
      ARC_GET_INSTANCE_FOR_METHOD(arc_bridge_service_->cert_store(), Init);
  DCHECK(instance);

  mojom::CertStoreHostPtr host_proxy;
  binding_.Bind(mojo::MakeRequest(&host_proxy));
  instance->Init(std::move(host_proxy));

  OnKeyPermissionsChanged();
}

void ArcCertStoreBridge::OnInstanceClosed() {
  VLOG(1) << "ArcCertStoreBridge::OnInstanceClosed";

  if (policy_service_ != nullptr) {
    policy_service_->RemoveObserver(policy::POLICY_DOMAIN_CHROME, this);
    policy_service_ = nullptr;
  }
}

void ArcCertStoreBridge::ListCertificates(
    const ListCertificatesCallback& callback) {
  VLOG(1) << "ArcCertStoreBridge::ListCertificates";

  std::vector<mojom::CertificatePtr> permitted_certs;
  if (!channel_enabled_) {
    LOG(WARNING) << "ARC certificate store channel is not enabled.";
    callback.Run(std::move(permitted_certs));
    return;
  }
  {
    CERTCertList* cert_list = PK11_ListCerts(PK11CertListUnique, nullptr);
    for (CERTCertListNode* node = CERT_LIST_HEAD(cert_list);
         !CERT_LIST_END(node, cert_list); node = CERT_LIST_NEXT(node)) {
      const SECItem& spki_der = node->cert->derPublicKey;
      if (chromeos::KeyPermissions::IsCorporateKey(
              std::string(spki_der.data, spki_der.data + spki_der.len),
              Profile::FromBrowserContext(context_)->GetPrefs())) {
        mojom::CertificatePtr certificate = mojom::Certificate::New();
        certificate->alias = node->cert->nickname;
        base::Base64Encode(
            std::string(node->cert->derCert.data,
                        node->cert->derCert.data + node->cert->derCert.len),
            &certificate->cert);

        permitted_certs.emplace_back(std::move(certificate));
      }
    }
    CERT_DestroyCertList(cert_list);
  }
  callback.Run(std::move(permitted_certs));
}

void ArcCertStoreBridge::GetKeyCharacteristics(
    mojom::KeyCharacteristicsRequestPtr request,
    const GetKeyCharacteristicsCallback& callback) {
  VLOG(1) << "ArcCertStoreBridge::GetKeyCharacteristics";

  auto response = mojom::KeyCharacteristicsResponse::New();
  response->error = mojom::Error::ERROR_UNIMPLEMENTED;

  if (!channel_enabled_) {
    LOG(WARNING) << "ARC certificate store channel is not enabled.";
    callback.Run(std::move(response));
    return;
  }

  if (!IsCertificateAllowed(
          request->alias, Profile::FromBrowserContext(context_)->GetPrefs())) {
    response->error = mojom::Error::ERROR_INVALID_KEY;
    callback.Run(std::move(response));
    return;
  }
  // TODO(pbond): implement.
  callback.Run(std::move(response));
}

void ArcCertStoreBridge::Begin(mojom::BeginOperationRequestPtr request,
                               const BeginCallback& callback) {
  VLOG(1) << "ArcCertStoreBridge::Begin";

  auto response = mojom::BeginOperationResponse::New();
  response->error = mojom::Error::ERROR_UNIMPLEMENTED;

  if (!channel_enabled_) {
    LOG(WARNING) << "ARC certificate store channel is not enabled.";
    callback.Run(std::move(response));
    return;
  }

  if (!IsCertificateAllowed(
          request->alias, Profile::FromBrowserContext(context_)->GetPrefs())) {
    response->error = mojom::Error::ERROR_INVALID_KEY;
    callback.Run(std::move(response));
    return;
  }
  // TODO(pbond): implement.
  callback.Run(std::move(response));
}

void ArcCertStoreBridge::Update(mojom::UpdateOperationRequestPtr request,
                                const UpdateCallback& callback) {
  VLOG(1) << "ArcCertStoreBridge::Update";

  auto response = mojom::UpdateOperationResponse::New();
  response->error = mojom::Error::ERROR_UNIMPLEMENTED;

  if (!channel_enabled_) {
    LOG(WARNING) << "ARC certificate store channel is not enabled.";
    callback.Run(std::move(response));
    return;
  }
  // TODO(pbond): implement.
  callback.Run(std::move(response));
}

void ArcCertStoreBridge::Finish(mojom::FinishOperationRequestPtr request,
                                const FinishCallback& callback) {
  VLOG(1) << "ArcCertStoreBridge::Finish";

  auto response = mojom::FinishOperationResponse::New();
  response->error = mojom::Error::ERROR_UNIMPLEMENTED;

  if (!channel_enabled_) {
    LOG(WARNING) << "ARC certificate store channel is not enabled.";
    callback.Run(std::move(response));
    return;
  }
  // TODO(pbond): implement.
  callback.Run(std::move(response));
}

void ArcCertStoreBridge::Abort(mojom::AbortOperationRequestPtr request,
                               const AbortCallback& callback) {
  VLOG(1) << "ArcCertStoreBridge::Abort";

  auto response = mojom::AbortOperationResponse::New();
  response->error = mojom::Error::ERROR_UNIMPLEMENTED;

  if (!channel_enabled_) {
    LOG(WARNING) << "ARC certificate store channel is not enabled.";
    callback.Run(std::move(response));
    return;
  }
  // TODO(pbond): implement.
  callback.Run(std::move(response));
}

void ArcCertStoreBridge::OnPolicyUpdated(const policy::PolicyNamespace& ns,
                                         const policy::PolicyMap& previous,
                                         const policy::PolicyMap& current) {
  VLOG(1) << "ArcCertStoreBridge::OnPolicyUpdated";

  OnKeyPermissionsChanged();
}

void ArcCertStoreBridge::OnKeyPermissionsChanged() {
  VLOG(1) << "ArcCertStoreBridge::OnKeyPermissionsChanged";

  channel_enabled_ = false;

  const policy::PolicyMap& policies = policy_service_->GetPolicies(
      policy::PolicyNamespace(policy::POLICY_DOMAIN_CHROME, std::string()));
  const base::Value* policy_value =
      policies.GetValue(policy::key::kKeyPermissions);
  if (!policy_value) {
    LOG(WARNING) << "KeyPermissions policy is not set";
    return;
  }

  const base::DictionaryValue* key_permissions_map = nullptr;
  policy_value->GetAsDictionary(&key_permissions_map);
  if (!key_permissions_map) {
    LOG(ERROR) << "Expected policy to be a dictionary.";
    return;
  }

  std::vector<mojom::KeyPermissionPtr> permissions;
  for (const auto& it : key_permissions_map->DictItems()) {
    if (IsAndroidPackageName(it.first)) {
      const base::DictionaryValue* key_permissions_for_app = nullptr;
      if (!it.second.GetAsDictionary(&key_permissions_for_app) &&
          !key_permissions_for_app) {
        continue;
      }
      bool allow_corporate_key_usage = false;
      key_permissions_for_app->GetBooleanWithoutPathExpansion(
          kPolicyAllowCorporateKeyUsage, &allow_corporate_key_usage);
      if (allow_corporate_key_usage) {
        mojom::KeyPermissionPtr permission = mojom::KeyPermission::New();
        permission->packageName = it.first;
        permissions.push_back(std::move(permission));
      }
    }
  }
  channel_enabled_ = !permissions.empty();

  auto* instance = ARC_GET_INSTANCE_FOR_METHOD(
      arc_bridge_service_->cert_store(), OnKeyPermissionsChanged);
  if (!instance) {
    LOG(ERROR) << "There is no ArcCertStoreInstance";
    return;
  }
  instance->OnKeyPermissionsChanged(std::move(permissions));
}

}  // namespace arc
