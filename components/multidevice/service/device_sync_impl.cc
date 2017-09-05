// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/multidevice/service/device_sync_impl.h"

#include "base/guid.h"
#include "base/sys_info.h"
#include "base/time/default_clock.h"
#include "base/version.h"
#include "chrome/browser/chromeos/login/easy_unlock/secure_message_delegate_chromeos.h"
#include "components/cryptauth/cryptauth_api_call_flow.h"
#include "components/cryptauth/cryptauth_client_impl.h"
#include "components/cryptauth/cryptauth_enroller_impl.h"
#include "components/cryptauth/cryptauth_enrollment_utils.h"
#include "components/cryptauth/proto/cryptauth_api.pb.h"
#include "components/cryptauth/remote_device_provider.h"
#include "components/prefs/pref_service.h"
#include "components/version_info/version_info.h"
#include "services/identity/public/interfaces/constants.mojom.h"
#include "services/service_manager/public/cpp/connector.h"

#if defined(OS_CHROMEOS)
#include "ash/shell.h"
#include "base/linux_util.h"
#include "chrome/browser/chromeos/login/easy_unlock/secure_message_delegate_chromeos.h"
#include "ui/display/manager/display_manager.h"
#include "ui/display/manager/managed_display_info.h"
#include "ui/gfx/geometry/rect.h"
#endif

namespace {

// TODO(hsuregan): Find out how to get pref_services_ properly.
PrefService* GetPrefServices() {
  return nullptr;
}

::identity::ScopeSet GetScopes() {
  identity::ScopeSet scopes;
  scopes.insert("https://www.googleapis.com/auth/cryptauth");
  return scopes;
}

// TODO(hsuregan): Find out how to get device id properly.
std::string GetDeviceId() {
  return base::GenerateGUID();
}

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

cryptauth::GcmDeviceInfo GetGcmDeviceInfo() {
  cryptauth::GcmDeviceInfo device_info;
  device_info.set_long_device_id(GetDeviceId());
  device_info.set_device_type(cryptauth::CHROME);
  device_info.set_device_software_version(version_info::GetVersionNumber());
  google::protobuf::int64 software_version_code =
      cryptauth::HashStringToInt64(version_info::GetLastChange());
  device_info.set_device_software_version_code(software_version_code);

#if defined(OS_CHROMEOS)
  device_info.set_device_model(base::SysInfo::GetLsbReleaseBoard());
  device_info.set_device_os_version(base::GetLinuxDistro());
  // The Chrome OS version tracks the Chrome version, so fill in the same value
  // as |device_software_version_code|.
  device_info.set_device_os_version_code(software_version_code);

  // There may not be a Shell instance in tests.
  if (!ash::Shell::HasInstance())
    return device_info;

  display::DisplayManager* display_manager =
      ash::Shell::Get()->display_manager();
  int64_t primary_display_id =
      display_manager->GetPrimaryDisplayCandidate().id();
  display::ManagedDisplayInfo display_info =
      display_manager->GetDisplayInfo(primary_display_id);
  gfx::Rect bounds = display_info.bounds_in_native();

  // TODO(tengs): This is a heuristic to deterimine the DPI of the display, as
  // there is no convenient way of getting this information right now.
  const double dpi = display_info.device_scale_factor() > 1.0f ? 239.0f : 96.0f;
  double width_in_inches = (bounds.width() - bounds.x()) / dpi;
  double height_in_inches = (bounds.height() - bounds.y()) / dpi;
  double diagonal_in_inches = sqrt(width_in_inches * width_in_inches +
                                   height_in_inches * height_in_inches);

  // Note: The unit of this measument is in milli-inches.
  device_info.set_device_display_diagonal_mils(diagonal_in_inches * 1000.0);
#else
// TODO(tengs): Fill in device information for other platforms.
#endif

  return device_info;
}

class CryptAuthAccessTokenFetcherImpl
    : public cryptauth::CryptAuthAccessTokenFetcher {
 public:
  CryptAuthAccessTokenFetcherImpl(
      AccountInfo account_info,
      identity::mojom::IdentityManagerPtr* identity_manager)
      : account_info_(account_info),
        identity_manager_(identity_manager),
        weak_ptr_factory_(this) {}

  void FetchAccessToken(const AccessTokenCallback& callback) override {
    (*identity_manager_)
        ->GetAccessToken(account_info_.account_id, GetScopes(),
                         account_info_.given_name,
                         base::Bind(&CryptAuthAccessTokenFetcherImpl::
                                        IdentityManagerGetAccessTokenCallback,
                                    weak_ptr_factory_.GetWeakPtr(), callback));
  }

  void IdentityManagerGetAccessTokenCallback(
      AccessTokenCallback callback,
      const base::Optional<std::string>& access_token,
      base::Time expiration_time,
      const ::GoogleServiceAuthError& error) {
    if (access_token.has_value()) {
      // Like cryptauth::CryptAuthAccessTokenFetcherImpl::OnGetTokenSuccess
      callback.Run(access_token.value());
      return;
    }
    // Like cryptauth::CryptAuthAccessTokenFetcherImpl::OnGetTokenFailure
    callback.Run(std::string());
  }

 private:
  AccountInfo account_info_;
  identity::mojom::IdentityManagerPtr* identity_manager_;
  base::WeakPtrFactory<CryptAuthAccessTokenFetcherImpl> weak_ptr_factory_;
};

class CryptAuthClientFactoryImpl : public cryptauth::CryptAuthClientFactory {
 public:
  CryptAuthClientFactoryImpl(
      identity::mojom::IdentityManagerPtr* identity_manager,
      AccountInfo account_info,
      scoped_refptr<net::URLRequestContextGetter> url_request_context,
      const cryptauth::DeviceClassifier& device_classifier)
      : identity_manager_(identity_manager),
        account_info_(account_info),
        url_request_context_(url_request_context),
        device_classifier_(device_classifier) {}

  ~CryptAuthClientFactoryImpl() override {}

  std::unique_ptr<cryptauth::CryptAuthClient> CreateInstance() override {
    return base::MakeUnique<cryptauth::CryptAuthClientImpl>(
        base::WrapUnique(new cryptauth::CryptAuthApiCallFlow()),
        base::WrapUnique(new CryptAuthAccessTokenFetcherImpl(
            account_info_, identity_manager_)),
        url_request_context_, device_classifier_);
  }

 private:
  identity::mojom::IdentityManagerPtr* identity_manager_;
  AccountInfo account_info_;
  scoped_refptr<net::URLRequestContextGetter> url_request_context_;
  const cryptauth::DeviceClassifier& device_classifier_;
};

class CryptAuthEnrollerFactoryImpl
    : public cryptauth::CryptAuthEnrollerFactory {
 public:
  CryptAuthEnrollerFactoryImpl(
      identity::mojom::IdentityManagerPtr* identity_manager,
      AccountInfo account_info,
      scoped_refptr<net::URLRequestContextGetter> url_request_context,
      cryptauth::SecureMessageDelegateFactory* secure_message_delegate_factory)
      : identity_manager_(identity_manager),
        account_info_(account_info),
        url_request_context_(url_request_context),
        secure_message_delegate_factory_(secure_message_delegate_factory) {}

  std::unique_ptr<cryptauth::CryptAuthEnroller> CreateInstance() override {
    return base::MakeUnique<cryptauth::CryptAuthEnrollerImpl>(
        base::MakeUnique<CryptAuthClientFactoryImpl>(
            identity_manager_, account_info_, url_request_context_,
            GetDeviceClassifierImpl()),
        secure_message_delegate_factory_->CreateSecureMessageDelegate());
  }

 private:
  identity::mojom::IdentityManagerPtr* identity_manager_;
  AccountInfo account_info_;
  scoped_refptr<net::URLRequestContextGetter> url_request_context_;
  cryptauth::SecureMessageDelegateFactory* secure_message_delegate_factory_;
};

}  // namespace

namespace multidevice {

DeviceSyncImpl::DeviceSyncImpl(
    std::unique_ptr<service_manager::ServiceContextRef> service_ref,
    scoped_refptr<net::URLRequestContextGetter> request_context,
    identity::mojom::IdentityManagerPtr identity_manager,
    cryptauth::CryptAuthGCMManager* gcm_manager,
    cryptauth::SecureMessageDelegateFactory* secure_message_delegate_factory)
    : service_ref_(std::move(service_ref)),
      identity_manager_(std::move(identity_manager)),
      gcm_manager_(gcm_manager),
      secure_message_delegate_factory_(secure_message_delegate_factory),
      managers_were_injected_(false),
      request_context_(std::move(request_context)),
      weak_ptr_factory_(this) {
  Initialize();
}

DeviceSyncImpl::DeviceSyncImpl(
    std::unique_ptr<service_manager::ServiceContextRef> service_ref,
    identity::mojom::IdentityManagerPtr identity_manager,
    cryptauth::CryptAuthGCMManager* gcm_manager,
    cryptauth::CryptAuthDeviceManager* device_manager,
    cryptauth::CryptAuthEnrollmentManager* enrollment_manager,
    cryptauth::SecureMessageDelegateFactory* secure_message_delegate_factory)
    : service_ref_(std::move(service_ref)),
      identity_manager_(std::move(identity_manager)),
      gcm_manager_(gcm_manager),
      enrollment_manager_(enrollment_manager),
      device_manager_(device_manager),
      secure_message_delegate_factory_(secure_message_delegate_factory),
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

void DeviceSyncImpl::AddObserver(
    device_sync::mojom::DeviceSyncObserverPtr observer) {
  observers_.AddPtr(std::move(observer));
}

void DeviceSyncImpl::OnEnrollmentStarted() {}

void DeviceSyncImpl::OnEnrollmentFinished(bool success) {
  observers_.ForAllPtrs(
      [&success](device_sync::mojom::DeviceSyncObserver* observer) {
        observer->OnEnrollmentFinished(success /* success */);
      });

  if (success)
    SetStateReadyAndSync();
}

void DeviceSyncImpl::OnSyncStarted() {}

void DeviceSyncImpl::OnSyncFinished(
    cryptauth::CryptAuthDeviceManager::SyncResult sync_result,
    cryptauth::CryptAuthDeviceManager::DeviceChangeResult
        device_change_result) {
  observers_.ForAllPtrs([&sync_result, &device_change_result](
                            device_sync::mojom::DeviceSyncObserver* observer) {
    observer->OnDevicesSynced(
        sync_result == cryptauth::CryptAuthDeviceManager::SyncResult::
                           SUCCESS /* success */,
        device_change_result ==
            cryptauth::CryptAuthDeviceManager::DeviceChangeResult::
                CHANGED /* device_change_result*/);
  });
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

void DeviceSyncImpl::SetStateReadyAndSync() {
  state_ = State::READY;
  GetDeviceManager()->AddObserver(this);
  ForceSyncInternal(true /* is_initializing */);
}

void DeviceSyncImpl::SetStateReadyAndEnroll() {
  state_ = State::READY;
  GetEnrollmentManager()->AddObserver(this);
  ForceSyncInternal(true /* is_initializing */);
}

void DeviceSyncImpl::BranchGenerateKeyOrSync() {
  if (!GetEnrollmentManager()->IsEnrollmentValid()) {
    state_ = State::WAITING_FOR_KEY_PAIR_GENERATION;
    secure_message_delegate_factory_->CreateSecureMessageDelegate()
        ->GenerateKeyPair(base::Bind(&DeviceSyncImpl::OnKeyPairGenerated,
                                     weak_ptr_factory_.GetWeakPtr()));
    return;
  } else {
    DCHECK(remote_device_provider_);
    SetStateReadyAndSync();
  }
}

void DeviceSyncImpl::OnPrimaryAccountAvailable(
    const AccountInfo& account_info,
    const identity::AccountState& account_state) {
  DCHECK(state_ == State::WAITING_FOR_PRIMARY_ACCOUNT);

  primary_account_info_ = account_info;
  primary_account_state_ = account_state;

  if (!managers_were_injected_) {
    state_ = State::CREATING_CRYPTAUTH_MANAGERS;
    device_manager_ = base::MakeUnique<cryptauth::CryptAuthDeviceManager>(
        base::MakeUnique<base::DefaultClock>(),
        base::MakeUnique<CryptAuthClientFactoryImpl>(
            &identity_manager_, primary_account_info_.value(), request_context_,
            GetDeviceClassifierImpl()),
        gcm_manager_.get(), GetPrefServices());
    enrollment_manager_ =
        base::MakeUnique<cryptauth::CryptAuthEnrollmentManager>(
            base::MakeUnique<base::DefaultClock>(),
            base::MakeUnique<CryptAuthEnrollerFactoryImpl>(
                &identity_manager_, primary_account_info_.value(),
                request_context_, secure_message_delegate_factory_.get()),
            secure_message_delegate_factory_->CreateSecureMessageDelegate(),
            GetGcmDeviceInfo(), gcm_manager_.get(), GetPrefServices());
  }
  BranchGenerateKeyOrSync();
}

void DeviceSyncImpl::OnKeyPairGenerated(const std::string& public_key,
                                        const std::string& private_key) {
  DCHECK(state_ == State::WAITING_FOR_KEY_PAIR_GENERATION);

  public_key_ = public_key;
  private_key_ = private_key;

  if (!primary_account_info_.has_value()) {
    return;
  }
  DCHECK(primary_account_info_->IsValid());
  DCHECK(!private_key_.empty());
  remote_device_provider_ = base::MakeUnique<cryptauth::RemoteDeviceProvider>(
      GetDeviceManager(), primary_account_info_->account_id, private_key_,
      secure_message_delegate_factory_.get());
  SetStateReadyAndEnroll();
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