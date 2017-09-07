// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/multidevice/service/multidevice_service.h"

#include "components/cryptauth/remote_device_provider.h"
#include "components/multidevice/service/device_sync_impl.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "services/service_manager/public/cpp/service_context.h"

namespace multidevice {

// TODO(hsuregan): Remove when whereabouts of parameters can be determined.
MultiDeviceService::MultiDeviceService()
    : MultiDeviceService(nullptr, nullptr, nullptr, nullptr, nullptr, nullptr) {
}

MultiDeviceService::MultiDeviceService(
    identity::mojom::IdentityManagerPtr identity_manager,
    prefs::mojom::PrefStoreConnectorPtr pref_connector,
    std::unique_ptr<cryptauth::CryptAuthGCMManager> gcm_manager,
    std::unique_ptr<cryptauth::CryptAuthDeviceManager> device_manager,
    std::unique_ptr<cryptauth::SecureMessageDelegateFactory>
        secure_message_delegate_factory,
    std::unique_ptr<cryptauth::CryptAuthEnrollmentManager> enrollment_manager)
    : identity_manager_(std::move(identity_manager)),
      pref_connector_(std::move(pref_connector)),
      gcm_manager_(std::move(gcm_manager)),
      device_manager_(std::move(device_manager)),
      enrollment_manager_(std::move(enrollment_manager)),
      secure_message_delegate_factory_(
          std::move(secure_message_delegate_factory)),
      weak_ptr_factory_(this) {}

MultiDeviceService::~MultiDeviceService() {}

void MultiDeviceService::OnStart() {
  ref_factory_ = base::MakeUnique<service_manager::ServiceContextRefFactory>(
      base::Bind(&service_manager::ServiceContext::RequestQuit,
                 base::Unretained(context())));
  registry_.AddInterface<device_sync::mojom::DeviceSync>(
      base::Bind(&MultiDeviceService::CreateDeviceSyncImpl,
                 base::Unretained(this), ref_factory_.get()));
}

void MultiDeviceService::OnBindInterface(
    const service_manager::BindSourceInfo& source_info,
    const std::string& interface_name,
    mojo::ScopedMessagePipeHandle interface_pipe) {
  registry_.BindInterface(interface_name, std::move(interface_pipe));
}

void MultiDeviceService::CreateDeviceSyncImpl(
    service_manager::ServiceContextRefFactory* ref_factory,
    device_sync::mojom::DeviceSyncRequest request) {
  mojo::MakeStrongBinding(
      base::MakeUnique<DeviceSyncImpl>(
          ref_factory->CreateRef(), std::move(identity_manager_),
          std::move(pref_connector_), gcm_manager_.get(), device_manager_.get(),
          enrollment_manager_.get(), secure_message_delegate_factory_.get()),
      std::move(request));
}

}  // namespace multidevice