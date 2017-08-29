// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/arc/enterprise/arc_cert_store_bridge.h"

#include <cert.h>
#include <pk11pub.h>

#include "base/base64.h"
#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/memory/singleton.h"
#include "chrome/browser/chromeos/platform_keys/key_permissions.h"
#include "chrome/browser/chromeos/profiles/profile_helper.h"
#include "chrome/browser/policy/profile_policy_connector.h"
#include "chrome/browser/policy/profile_policy_connector_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "components/arc/arc_bridge_service.h"
#include "components/arc/arc_browser_context_keyed_service_factory_base.h"
#include "components/arc/arc_service_manager.h"
#include "components/policy/policy_constants.h"
#include "components/user_manager/user.h"
#include "components/user_manager/user_manager.h"
#include "content/public/browser/browser_thread.h"

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

}  // namespace

// static
ArcCertStoreBridge* ArcCertStoreBridge::GetForBrowserContext(
    content::BrowserContext* context) {
  return ArcCertStoreBridgeFactory::GetForBrowserContext(context);
}

ArcCertStoreBridge::ArcCertStoreBridge(content::BrowserContext* context,
                                       ArcBridgeService* bridge_service)
    : arc_bridge_service_(bridge_service),
      binding_(this),
      weak_ptr_factory_(this) {
  VLOG(1) << "ArcCertStoreBridge::Init";

  arc_bridge_service_->cert_store()->AddObserver(this);
}

ArcCertStoreBridge::~ArcCertStoreBridge() {
  VLOG(1) << "ArcCertStoreBridge::Destroy";

  arc_bridge_service_->cert_store()->RemoveObserver(this);
}

void ArcCertStoreBridge::OnInstanceReady() {
  VLOG(1) << "ArcCertStoreBridge::OnInstanceReady";

  const user_manager::User* const primary_user =
      user_manager::UserManager::Get()->GetPrimaryUser();
  Profile* const profile =
      chromeos::ProfileHelper::Get()->GetProfileByUser(primary_user);
  if (policy_service_ == nullptr) {
    auto* profile_policy_connector =
        policy::ProfilePolicyConnectorFactory::GetForBrowserContext(profile);
    policy_service_ = profile_policy_connector->policy_service();
  }

  if (pref_service_ == nullptr)
    pref_service_ = profile->GetPrefs();

  policy_service_->AddObserver(policy::POLICY_DOMAIN_CHROME, this);

  auto* instance =
      ARC_GET_INSTANCE_FOR_METHOD(arc_bridge_service_->cert_store(), Init);
  DCHECK(instance);
  mojom::ArcCertStoreHostPtr host_proxy;
  binding_.Bind(mojo::MakeRequest(&host_proxy));
  instance->Init(std::move(host_proxy));

  content::BrowserThread::PostTask(
      content::BrowserThread::UI, FROM_HERE,
      base::Bind(&ArcCertStoreBridge::OnKeyPermissionsChanged,
                 weak_ptr_factory_.GetWeakPtr()));
}

void ArcCertStoreBridge::OnInstanceClosed() {
  VLOG(1) << "ArcCertStoreBridge::OnInstanceClosed";

  policy_service_->RemoveObserver(policy::POLICY_DOMAIN_CHROME, this);
  policy_service_ = nullptr;
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
  CERTCertList* cert_list = PK11_ListCerts(PK11CertListUnique, NULL);
  CERTCertListNode* node;
  for (node = CERT_LIST_HEAD(cert_list); !CERT_LIST_END(node, cert_list);
       node = CERT_LIST_NEXT(node)) {
    const SECItem& spki_der = node->cert->derPublicKey;
    if (chromeos::KeyPermissions::IsCorporateKey(
            std::string(spki_der.data, spki_der.data + spki_der.len),
            pref_service_)) {
      mojom::CertificatePtr certificate = mojom::Certificate::New();
      certificate->alias = node->cert->nickname;
      std::string cert_der_b64;
      base::Base64Encode(
          std::string(node->cert->derCert.data,
                      node->cert->derCert.data + node->cert->derCert.len),
          &certificate->cert);

      permitted_certs.emplace_back(certificate.Clone());
    }
  }
  CERT_DestroyCertList(cert_list);
  callback.Run(std::move(permitted_certs));
}

void ArcCertStoreBridge::SendRequest(
    mojom::Command cmd,
    mojo::ScopedSharedBufferHandle request_handle,
    uint32_t size,
    mojo::ScopedSharedBufferHandle response_handle,
    const SendRequestCallback& callback) {
  VLOG(1) << "ArcCertStoreBridge::SendRequest cmd=" << cmd;

  uint32_t rcv_size = 0;

  if (!channel_enabled_) {
    LOG(WARNING) << "ARC certificate store channel is not enabled.";
    callback.Run(std::move(response_handle), std::move(rcv_size));
    return;
  }
  switch (cmd) {
    case mojom::Command::BEGIN:
    case mojom::Command::UPDATE:
    case mojom::Command::ABORT:
    case mojom::Command::FINISH:
    case mojom::Command::GET_KEY_CHARACTERISTICS:
    // TODO(pbond): add command processing.
    default:
      rcv_size = 0;
  }
  callback.Run(std::move(response_handle), std::move(rcv_size));
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
  mojom::KeyPermissionPtr permission = mojom::KeyPermission::New();
  permission->packageName = "com.example.android.keychain";
  permissions.emplace_back(permission.Clone());
  for (base::DictionaryValue::Iterator it(*key_permissions_map); !it.IsAtEnd();
       it.Advance()) {
    if (IsAndroidPackageName(it.key())) {
      const base::DictionaryValue* key_permissions_for_app = nullptr;
      if (!it.value().GetAsDictionary(&key_permissions_for_app) &&
          !key_permissions_for_app) {
        continue;
      }
      bool allow_corporate_key_usage = false;
      key_permissions_for_app->GetBooleanWithoutPathExpansion(
          kPolicyAllowCorporateKeyUsage, &allow_corporate_key_usage);
      if (allow_corporate_key_usage) {
        mojom::KeyPermissionPtr permission = mojom::KeyPermission::New();
        permission->packageName = it.key();
        permissions.push_back(std::move(permission));
      }
    }
  }
  channel_enabled_ = permissions.size() > 0;

  auto* instance = ARC_GET_INSTANCE_FOR_METHOD(
      arc_bridge_service_->cert_store(), OnKeyPermissionsChanged);
  if (!instance) {
    LOG(ERROR) << "There is no ArcCertStoreInstance";
    return;
  }
  instance->OnKeyPermissionsChanged(std::move(permissions));
}

}  // namespace arc
