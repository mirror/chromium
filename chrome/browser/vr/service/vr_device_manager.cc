// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/vr/service/vr_device_manager.h"

#include <utility>

#include "base/bind.h"
#include "base/memory/ptr_util.h"
#include "base/memory/singleton.h"
#include "build/build_config.h"
#include "chrome/common/chrome_features.h"
#include "content/public/common/service_manager_connection.h"
#include "device/vr/features/features.h"
#include "device/vr/orientation/orientation_device_provider.h"

#if defined(OS_ANDROID)
#include "device/vr/android/gvr/gvr_device_provider.h"
#endif

#if BUILDFLAG(ENABLE_OPENVR)
#include "device/vr/openvr/openvr_device_provider.h"
#endif

namespace vr {

namespace {
VRDeviceManager* g_vr_device_manager = nullptr;

void ProvideDevice(VRServiceImpl* service,
                   base::OnceClosure callback,
                   device::VRDevice* device) {
  service->ConnectDevice(device);
  std::move(callback).Run();
}

}  // namespace

VRDeviceManager* VRDeviceManager::GetInstance() {
  if (!g_vr_device_manager) {
    // Register VRDeviceProviders for the current platform
    ProviderList providers;
#if defined(OS_ANDROID)
    providers.emplace_back(std::make_unique<device::GvrDeviceProvider>());
#endif

#if BUILDFLAG(ENABLE_OPENVR)
    if (base::FeatureList::IsEnabled(features::kOpenVR))
      providers.emplace_back(std::make_unique<device::OpenVRDeviceProvider>());
#endif
    new VRDeviceManager(std::move(providers));
  }
  return g_vr_device_manager;
}

bool VRDeviceManager::HasInstance() {
  return g_vr_device_manager != nullptr;
}

VRDeviceManager::VRDeviceManager(ProviderList providers)
    : providers_(std::move(providers)) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

#if defined(OS_ANDROID)
  content::ServiceManagerConnection* connection =
      content::ServiceManagerConnection::GetForProcess();
  if (connection) {
    default_provider_ = std::make_unique<device::OrientationDeviceProvider>(
        connection->GetConnector());
  }
#endif

  CHECK(!g_vr_device_manager);
  g_vr_device_manager = this;
}

VRDeviceManager::~VRDeviceManager() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  g_vr_device_manager = nullptr;
}

void VRDeviceManager::AddService(
    VRServiceImpl* service,
    device::mojom::VRService::SetClientCallback callback) {
  // Loop through any currently active devices and send Connected messages to
  // the service. Future devices that come online will send a Connected message
  // when they are created.
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

  InitializeProviders();

  std::vector<device::VRDevice*> devices;
  for (const auto& provider : providers_)
    provider->GetDevices(&devices);

  for (auto* device : devices) {
    if (device->GetId() == device::VR_DEVICE_LAST_ID)
      continue;

    if (devices_.find(device->GetId()) == devices_.end())
      devices_[device->GetId()] = device;

    service->ConnectDevice(device);
  }

  services_.insert(service);

  // If there are no devices, try to at least provide the orientation API.
  if (devices.size() == 0 && default_provider_) {
    default_provider_->GetDevice(
        base::BindOnce(&ProvideDevice, service, std::move(callback)));
  } else {
    std::move(callback).Run();
  }
}

void VRDeviceManager::RemoveService(VRServiceImpl* service) {
  services_.erase(service);

  if (services_.empty()) {
    // Delete the device manager when it has no active connections.
    delete g_vr_device_manager;
  }
}

device::VRDevice* VRDeviceManager::GetDevice(unsigned int index) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

  if (index == 0)
    return nullptr;

  DeviceMap::iterator iter = devices_.find(index);
  if (iter == devices_.end())
    return nullptr;

  return iter->second;
}

void VRDeviceManager::InitializeProviders() {
  if (vr_initialized_)
    return;

  for (const auto& provider : providers_)
    provider->Initialize();

  if (default_provider_) {
    default_provider_->Initialize();
  }

  vr_initialized_ = true;
}

size_t VRDeviceManager::NumberOfConnectedServices() {
  return services_.size();
}

}  // namespace vr
