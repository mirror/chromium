// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/multidevice/service/device_sync_impl.h"

#include "base/base64.h"
#include "base/sys_info.h"
#include "base/time/default_clock.h"
#include "base/version.h"
#include "components/cryptauth/proto/cryptauth_api.pb.h"
#include "components/cryptauth/remote_device_provider.h"
#include "components/cryptauth/secure_message_delegate.h"
#include "components/multidevice/service/cryptauth_client_factory_impl.h"
#include "components/multidevice/service/cryptauth_enroller_factory_impl.h"
#include "components/multidevice/service/cryptauth_token_fetcher_impl.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/pref_service_factory.h"
#include "components/version_info/version_info.h"
#include "net/url_request/url_request_context_getter.h"
#include "services/identity/public/interfaces/constants.mojom.h"
#include "services/preferences/public/cpp/pref_service_factory.h"
#include "services/service_manager/public/cpp/service_context_ref.h"

namespace {

const char kSuccessCode[] = "SUCCESS";
const char kErrorInternal[] = "ERROR_INTERNAL";
const char kErrorNoValidAccessToken[] = "ERROR_NO_VALID_ACCESS_TOKEN";
const char kErrorServerFailedToRespond[] = "ERROR_SERVER_FAILED_TO_RESPOND";
const char kErrorCannotParseServerResponse[] =
    "ERROR_CANNOT_PARSE_SERVER_RESPONSE";

cryptauth::DeviceClassifier GetDeviceClassifierImpl() {
  cryptauth::DeviceClassifier device_classifier;

#if defined(OS_CHROMEOS)
  int32_t major_version;
  int32_t minor_version;
  int32_t bugfix_version;
  // TODO(tengs): base::OperatingSystemVersionNumbers only works for ChromeOS.
  // We need to get different numbers for other platforms.
  base::SysInfo::OperatingSystemVersionNumbers(&major_version, &minor_version,
                                               &bugfix_version);
  device_classifier.set_device_os_version_code(major_version);
  device_classifier.set_device_type(cryptauth::CHROME);
#endif

  const std::vector<uint32_t> version_components =
      base::Version(version_info::GetVersionNumber()).components();
  if (!version_components.empty())
    device_classifier.set_device_software_version_code(version_components[0]);

  device_classifier.set_device_software_package(version_info::GetProductName());
  return device_classifier;
}

// TODO(khorimoto): Please inject this in this CL.
cryptauth::GcmDeviceInfo GetGcmDeviceInfo() {
  cryptauth::GcmDeviceInfo device_info;
  return device_info;
}

// TODO(khorimoto): Please inject this in this CL.
std::string GetDeviceID(const std::string public_key) {
  std::string device_id;
  base::Base64Encode(public_key, &device_id);
  return device_id;
}

device_sync::mojom::ResultCode ConvertToMojomResultCode(
    std::string result_code) {
  if (result_code == kSuccessCode) {
    return device_sync::mojom::ResultCode::SUCCESS;
  } else if (result_code == kErrorInternal) {
    return device_sync::mojom::ResultCode::ERROR_INTERNAL;
  } else if (result_code == kErrorNoValidAccessToken) {
    return device_sync::mojom::ResultCode::ERROR_NO_VALID_ACCESS_TOKEN;
  } else if (result_code == kErrorServerFailedToRespond) {
    return device_sync::mojom::ResultCode::ERROR_SERVER_FAILED_TO_RESPOND;
  } else if (result_code == kErrorCannotParseServerResponse) {
    return device_sync::mojom::ResultCode::ERROR_CANNOT_PARSE_SERVER_RESPONSE;
  }
  NOTREACHED();
  return device_sync::mojom::ResultCode::ERROR_INTERNAL;
}

}  // namespace

namespace multidevice {

DeviceSyncImpl::DeviceSyncImpl(
    std::unique_ptr<service_manager::ServiceContextRef> service_ref,
    scoped_refptr<net::URLRequestContextGetter> request_context,
    identity::mojom::IdentityManagerPtr identity_manager,
    prefs::mojom::PrefStoreConnectorPtr pref_connector,
    cryptauth::CryptAuthGCMManager* gcm_manager,
    cryptauth::SecureMessageDelegateFactory* secure_message_delegate_factory)
    : request_context_(std::move(request_context)),
      secure_message_delegate_factory_(secure_message_delegate_factory),
      service_ref_(std::move(service_ref)),
      pref_connector_(std::move(pref_connector)),
      identity_manager_(std::move(identity_manager)),
      gcm_manager_(gcm_manager),
      managers_were_injected_(false),
      weak_ptr_factory_(this) {
  Initialize();
}

DeviceSyncImpl::DeviceSyncImpl(
    std::unique_ptr<service_manager::ServiceContextRef> service_ref,
    identity::mojom::IdentityManagerPtr identity_manager,
    prefs::mojom::PrefStoreConnectorPtr pref_connector,
    cryptauth::CryptAuthGCMManager* gcm_manager,
    cryptauth::CryptAuthDeviceManager* device_manager,
    cryptauth::CryptAuthEnrollmentManager* enrollment_manager,
    cryptauth::SecureMessageDelegateFactory* secure_message_delegate_factory)
    : secure_message_delegate_factory_(secure_message_delegate_factory),
      service_ref_(std::move(service_ref)),
      pref_connector_(std::move(pref_connector)),
      identity_manager_(std::move(identity_manager)),
      gcm_manager_(gcm_manager),
      enrollment_manager_(enrollment_manager),
      device_manager_(device_manager),
      managers_were_injected_(true),
      weak_ptr_factory_(this) {
  Initialize();
}

DeviceSyncImpl::~DeviceSyncImpl() {
  GetEnrollmentManager()->RemoveObserver(this);
  GetDeviceManager()->RemoveObserver(this);
}

void DeviceSyncImpl::ForceEnrollmentNow() {
  ForceEnrollmentInternal(false /* is_initializing */);
}

void DeviceSyncImpl::ForceSyncNow() {
  ForceSyncInternal(false /* is_initializing */);
}

void DeviceSyncImpl::GetSyncedDevices(GetSyncedDevicesCallback callback) {
  std::move(callback).Run(remote_device_provider_->GetSyncedDevices());
}

void DeviceSyncImpl::SetCapabilityEnabled(
    const std::string& device_id,
    cryptauth::DeviceCapabilityManager::Capability capability,
    bool enabled,
    SetCapabilityEnabledCallback callback) {
  device_capability_manager_->SetCapabilityEnabled(
      GetEnrollmentManager()->GetUserPublicKey(), capability, enabled,
      base::Bind(&DeviceSyncImpl::CapabilityEnabledCallback,
                 weak_ptr_factory_.GetWeakPtr(), base::Passed(&callback),
                 kSuccessCode),
      base::Bind(&DeviceSyncImpl::CapabilityEnabledCallback,
                 weak_ptr_factory_.GetWeakPtr(), base::Passed(&callback)));
}

void DeviceSyncImpl::FindEligibleDevicesForCapability(
    cryptauth::DeviceCapabilityManager::Capability capability,
    FindEligibleDevicesForCapabilityCallback callback) {
  device_capability_manager_->FindEligibleDevicesForCapability(
      capability,
      base::Bind(&DeviceSyncImpl::SuccessEligibleDevicesForCapabilityCallback,
                 weak_ptr_factory_.GetWeakPtr(), base::Passed(&callback)),
      base::Bind(&DeviceSyncImpl::ErrorEligibleDevicesForCapabilityCallback,
                 weak_ptr_factory_.GetWeakPtr(), base::Passed(&callback)));
}

void DeviceSyncImpl::IsCapabilityPromotable(
    const std::string& device_id,
    cryptauth::DeviceCapabilityManager::Capability capability,
    IsCapabilityPromotableCallback callback) {
  device_capability_manager_->IsCapabilityPromotable(
      GetDeviceID(device_id), capability,
      base::Bind(&DeviceSyncImpl::SuccessCapabilityPromotableCallback,
                 weak_ptr_factory_.GetWeakPtr(), base::Passed(&callback)),
      base::Bind(&DeviceSyncImpl::ErrorCapabilityPromotableCallback,
                 weak_ptr_factory_.GetWeakPtr(), base::Passed(&callback)));
}

void DeviceSyncImpl::OnEnrollmentStarted() {}

void DeviceSyncImpl::OnEnrollmentFinished(bool success) {
  if (state_ == State::WAITING_FOR_ENROLLMENT_MANAGER_INIT && success)
    FinishPostEnrollmentInitialization();

  observers_.ForAllPtrs(
      [&success](device_sync::mojom::DeviceSyncObserver* observer) {
        observer->OnEnrollmentFinished(success);
      });
}

void DeviceSyncImpl::OnSyncStarted() {}

void DeviceSyncImpl::OnSyncFinished(
    cryptauth::CryptAuthDeviceManager::SyncResult sync_result,
    cryptauth::CryptAuthDeviceManager::DeviceChangeResult
        device_change_result) {
  observers_.ForAllPtrs([&sync_result, &device_change_result](
                            device_sync::mojom::DeviceSyncObserver* observer) {
    observer->OnDevicesSynced(
        sync_result == cryptauth::CryptAuthDeviceManager::SyncResult::SUCCESS,
        device_change_result ==
            cryptauth::CryptAuthDeviceManager::DeviceChangeResult::
                CHANGED /* device_change_result*/);
  });
}

void DeviceSyncImpl::GetUserPublicKey(GetUserPublicKeyCallback callback) {
  std::move(callback).Run(GetEnrollmentManager()->GetUserPublicKey());
}

void DeviceSyncImpl::AddObserver(
    device_sync::mojom::DeviceSyncObserverPtr observer) {
  observers_.AddPtr(std::move(observer));
}

cryptauth::CryptAuthEnrollmentManager* DeviceSyncImpl::GetEnrollmentManager() {
  if (enrollment_manager_) {
    DCHECK(!enrollment_manager_raw_);
    return enrollment_manager_.get();
  }

  DCHECK(enrollment_manager_raw_);
  return enrollment_manager_raw_;
}

cryptauth::CryptAuthDeviceManager* DeviceSyncImpl::GetDeviceManager() {
  if (device_manager_) {
    DCHECK(!device_manager_raw_);
    return device_manager_.get();
  }

  DCHECK(device_manager_raw_);
  return device_manager_raw_;
}

void DeviceSyncImpl::Initialize() {
  gcm_manager_->StartListening();

  state_ = State::WAITING_FOR_PRIMARY_ACCOUNT;
  identity_manager_->GetPrimaryAccountWhenAvailable(
      base::Bind(&DeviceSyncImpl::OnPrimaryAccountAvailable,
                 weak_ptr_factory_.GetWeakPtr()));
}

void DeviceSyncImpl::ConnectToPrefService() {
  state_ = State::WAITING_FOR_PREFS;
  auto registry = base::MakeRefCounted<user_prefs::PrefRegistrySyncable>();
  cryptauth::CryptAuthService::RegisterProfilePrefs(registry.get());
  prefs::ConnectToPrefService(
      std::move(pref_connector_), std::move(registry),
      base::Bind(&DeviceSyncImpl::OnConnectedToPrefService,
                 weak_ptr_factory_.GetWeakPtr()));
}

void DeviceSyncImpl::CreateManagers() {
  device_manager_ = base::MakeUnique<cryptauth::CryptAuthDeviceManager>(
      base::MakeUnique<base::DefaultClock>(),
      base::MakeUnique<CryptAuthClientFactoryImpl>(
          &identity_manager_, primary_account_info_, request_context_,
          GetDeviceClassifierImpl()),
      gcm_manager_.get(), pref_service_.get());

  enrollment_manager_ = base::MakeUnique<cryptauth::CryptAuthEnrollmentManager>(
      base::MakeUnique<base::DefaultClock>(),
      base::MakeUnique<CryptAuthEnrollerFactoryImpl>(
          &identity_manager_, primary_account_info_, GetDeviceClassifierImpl(),
          request_context_, secure_message_delegate_factory_),
      secure_message_delegate_factory_->CreateSecureMessageDelegate(),
      GetGcmDeviceInfo(), gcm_manager_.get(), pref_service_.get());
}

void DeviceSyncImpl::StartManagers() {
  state_ = State::WAITING_FOR_ENROLLMENT_MANAGER_INIT;

  GetDeviceManager()->AddObserver(this);
  GetEnrollmentManager()->AddObserver(this);

  if (GetEnrollmentManager()->IsEnrollmentValid()) {
    FinishPostEnrollmentInitialization();
    return;
  }

  state_ = State::WAITING_FOR_ENROLLMENT_MANAGER_INIT;
  ForceEnrollmentInternal(true /* is_initializing */);
}

void DeviceSyncImpl::FinishPostEnrollmentInitialization() {
  remote_device_provider_ = base::MakeUnique<cryptauth::RemoteDeviceProvider>(
      GetDeviceManager(), primary_account_info_.account_id,
      GetEnrollmentManager()->GetUserPrivateKey(),
      secure_message_delegate_factory_);

  device_capability_manager_ =
      base::MakeUnique<cryptauth::DeviceCapabilityManager>(
          new CryptAuthClientFactoryImpl(
              &identity_manager_, primary_account_info_, request_context_,
              GetDeviceClassifierImpl()));

  state_ = State::READY;
  ForceSyncInternal(true /* is_initializing */);
}

void DeviceSyncImpl::OnPrimaryAccountAvailable(
    const AccountInfo& account_info,
    const identity::AccountState& account_state) {
  DCHECK(state_ == State::WAITING_FOR_PRIMARY_ACCOUNT);

  primary_account_info_ = account_info;
  primary_account_state_ = account_state;

  if (managers_were_injected_) {
    StartManagers();
    return;
  }

  ConnectToPrefService();
}

void DeviceSyncImpl::OnConnectedToPrefService(
    std::unique_ptr<PrefService> pref_service) {
  DCHECK(state_ == State::WAITING_FOR_PREFS);
  pref_service_ = std::move(pref_service);
  CreateManagers();
  StartManagers();
}

void DeviceSyncImpl::CapabilityEnabledCallback(
    SetCapabilityEnabledCallback callback,
    const std::string& response) {
  std::move(callback).Run(device_sync::mojom::SetCapabilityResponse::New(
      ConvertToMojomResultCode(response)));
}

void DeviceSyncImpl::SuccessEligibleDevicesForCapabilityCallback(
    FindEligibleDevicesForCapabilityCallback callback,
    const std::vector<cryptauth::ExternalDeviceInfo>& eligible_devices,
    const std::vector<cryptauth::IneligibleDevice>& ineligible_devices) {
  std::vector<std::string> eligible_device_ids;
  for (const auto& eligible_device : eligible_devices) {
    eligible_device_ids.emplace_back(GetDeviceID(eligible_device.public_key()));
  }
  std::move(callback).Run(device_sync::mojom::FindEligibleDevicesResponse::New(
      ConvertToMojomResultCode(kSuccessCode), eligible_device_ids,
      ineligible_devices));
}

void DeviceSyncImpl::ErrorEligibleDevicesForCapabilityCallback(
    FindEligibleDevicesForCapabilityCallback callback,
    const std::string& error_code) {
  std::move(callback).Run(device_sync::mojom::FindEligibleDevicesResponse::New(
      ConvertToMojomResultCode(error_code), std::vector<std::string>(),
      std::vector<cryptauth::IneligibleDevice>()));
}

void DeviceSyncImpl::SuccessCapabilityPromotableCallback(
    IsCapabilityPromotableCallback callback,
    bool is_promotable) {
  std::move(callback).Run(
      device_sync::mojom::IsCapabilityPromotableResponse::New(
          ConvertToMojomResultCode(kSuccessCode), is_promotable));
}

void DeviceSyncImpl::ErrorCapabilityPromotableCallback(
    IsCapabilityPromotableCallback callback,
    const std::string& result_code) {
  std::move(callback).Run(
      device_sync::mojom::IsCapabilityPromotableResponse::New(
          ConvertToMojomResultCode(result_code), false /* is_promotable */));
}

void DeviceSyncImpl::ForceEnrollmentInternal(bool is_initializing) {
  GetEnrollmentManager()->ForceEnrollmentNow(
      is_initializing
          ? cryptauth::InvocationReason::INVOCATION_REASON_INITIALIZATION
          : cryptauth::InvocationReason::INVOCATION_REASON_MANUAL);
}

void DeviceSyncImpl::ForceSyncInternal(bool is_initializing) {
  GetDeviceManager()->ForceSyncNow(
      is_initializing
          ? cryptauth::InvocationReason::INVOCATION_REASON_INITIALIZATION
          : cryptauth::InvocationReason::INVOCATION_REASON_MANUAL);
}

}  // namespace multidevice