// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/arc/usb/usb_host_bridge.h"

#include <string>
#include <vector>

#include "base/bind.h"
#include "base/logging.h"
#include "base/memory/singleton.h"

#include "chromeos/dbus/dbus_thread_manager.h"
#include "chromeos/dbus/permission_broker_client.h"
#include "components/arc/arc_bridge_service.h"
#include "components/arc/arc_browser_context_keyed_service_factory_base.h"
#include "device/base/device_client.h"
#include "device/usb/mojo/type_converters.h"
#include "device/usb/usb_device.h"
#include "device/usb/usb_device_handle.h"
#include "device/usb/usb_device_linux.h"
#include "device/usb/usb_service.h"
#include "mojo/edk/embedder/embedder.h"
#include "mojo/edk/embedder/scoped_platform_handle.h"

namespace arc {
namespace {

// Singleton factory for UsbHostBridge
class UsbHostBridgeFactory
    : public internal::ArcBrowserContextKeyedServiceFactoryBase<
          UsbHostBridge,
          UsbHostBridgeFactory> {
 public:
  // Factory name used by ArcBrowserContextKeyedServiceFactoryBase.
  static constexpr const char* kName = "UsbHostBridgeFactory";

  static UsbHostBridgeFactory* GetInstance() {
    return base::Singleton<UsbHostBridgeFactory>::get();
  }

 private:
  friend base::DefaultSingletonTraits<UsbHostBridgeFactory>;
  UsbHostBridgeFactory() = default;
  ~UsbHostBridgeFactory() override = default;
};

}  // namespace

UsbHostBridge* UsbHostBridge::GetForBrowserContext(
    content::BrowserContext* context) {
  return UsbHostBridgeFactory::GetForBrowserContext(context);
}

device::UsbService* GetUsbService() {
  return device::DeviceClient::Get()->GetUsbService();
}

UsbHostBridge::UsbHostBridge(content::BrowserContext* context,
                             ArcBridgeService* bridge_service)
    : arc_bridge_service_(bridge_service),
      binding_(this),
      usb_observer_(this),
      weak_factory_(this) {
  arc_bridge_service_->usb_host()->AddObserver(this);

  device::UsbService* usb_service =
      device::DeviceClient::Get()->GetUsbService();
  DCHECK(usb_service);
  usb_observer_.Add(usb_service);
}

UsbHostBridge::~UsbHostBridge() {
  arc_bridge_service_->usb_host()->RemoveObserver(this);
  GetUsbService()->RemoveObserver(this);
}

void UsbHostBridge::OnInstanceReady() {
  mojom::UsbHostInstance* usb_host_instance =
      ARC_GET_INSTANCE_FOR_METHOD(arc_bridge_service_->usb_host(), Init);
  DCHECK(usb_host_instance);

  mojom::UsbHostPtr host_proxy;
  binding_.Bind(mojo::MakeRequest(&host_proxy));
  usb_host_instance->Init(std::move(host_proxy));
  binding_.set_connection_error_handler(
      base::Bind(&mojo::Binding<UsbHost>::Close, base::Unretained(&binding_)));

  // Send the (filtered) list of already existing USB devices to the other side.
  device::UsbService* service = GetUsbService();
  if (!service)
    return;

  service->GetDevices(base::Bind(&UsbHostBridge::OnGetDevicesComplete,
                                 weak_factory_.GetWeakPtr()));
}

void UsbHostBridge::DoRequestUserAuthorization(
    const std::string& guid,
    const std::string& package,
    RequestPermissionCallback callback) {
  // TODO: implement the UI dialog
  // fail close for now
  std::move(callback).Run(false);
}

bool UsbHostBridge::HasPermissionForDevice(const std::string& guid) {
  // TODO: implement permission settings
  // fail close for now
  return false;
}

void UsbHostBridge::RequestPermission(const std::string& guid,
                                      const std::string& package,
                                      bool interactive,
                                      RequestPermissionCallback callback) {
  VLOG(2) << "USB RequestPermission " << guid << " package " << package;
  // Permission already requested.
  if (HasPermissionForDevice(guid)) {
    std::move(callback).Run(true);
    return;
  }

  // The other side was just checking, fail without asking the user.
  if (!interactive) {
    std::move(callback).Run(false);
    return;
  }

  // Ask the authorization from the user.
  DoRequestUserAuthorization(guid, package, std::move(callback));
}

void UsbHostBridge::OnDeviceOpened(OpenDeviceCallback callback,
                                   base::ScopedFD fd) {
  DCHECK(fd.is_valid());
  mojo::edk::ScopedPlatformHandle platform_handle{
      mojo::edk::PlatformHandle(fd.release())};
  MojoHandle wrapped_handle;
  MojoResult wrap_result = mojo::edk::CreatePlatformHandleWrapper(
      std::move(platform_handle), &wrapped_handle);
  if (wrap_result != MOJO_RESULT_OK) {
    LOG(ERROR) << "Failed to wrap device FD. Closing: " << wrap_result;
    std::move(callback).Run(mojo::ScopedHandle());
    return;
  }
  mojo::ScopedHandle scoped_handle{mojo::Handle(wrapped_handle)};
  std::move(callback).Run(std::move(scoped_handle));
}

void UsbHostBridge::OnDeviceOpenError(OpenDeviceCallback callback,
                                      const std::string& error_name,
                                      const std::string& error_message) {
  LOG(WARNING) << "Cannot open USB device: " << error_name << ": "
               << error_message;
  std::move(callback).Run(mojo::ScopedHandle());
}

void UsbHostBridge::OpenDevice(const std::string& guid,
                               OpenDeviceCallback callback) {
  device::UsbService* service = GetUsbService();
  if (!service) {
    std::move(callback).Run(mojo::ScopedHandle());
    return;
  }

  device::UsbDeviceLinux* device =
      static_cast<device::UsbDeviceLinux*>(service->GetDevice(guid).get());
  if (!device) {
    std::move(callback).Run(mojo::ScopedHandle());
    return;
  }

  // The RequestPermission was never done, abort.
  if (!HasPermissionForDevice(guid)) {
    std::move(callback).Run(mojo::ScopedHandle());
    return;
  }

  chromeos::PermissionBrokerClient* client =
      chromeos::DBusThreadManager::Get()->GetPermissionBrokerClient();
  DCHECK(client) << "Could not get permission broker client.";
  client->OpenPath(
      device->device_path(),
      base::Bind(&UsbHostBridge::OnDeviceOpened, weak_factory_.GetWeakPtr(),
                 base::Passed(std::move(callback))),
      base::Bind(&UsbHostBridge::OnDeviceOpenError, weak_factory_.GetWeakPtr(),
                 base::Passed(std::move(callback))));
}

void UsbHostBridge::OnDeviceChecked(const std::string& guid, bool allowed) {
  if (allowed) {
    mojom::UsbHostInstance* usb_host_instance = ARC_GET_INSTANCE_FOR_METHOD(
        arc_bridge_service_->usb_host(), OnDeviceAdded);

    if (!usb_host_instance)
      return;

    usb_host_instance->OnDeviceAdded(guid);
  }
}

void UsbHostBridge::OnGetDevicesComplete(
    const std::vector<scoped_refptr<device::UsbDevice>>& devices) {
  device::UsbService* service = GetUsbService();
  if (!service)
    return;

  for (const scoped_refptr<device::UsbDevice>& device : devices) {
    device->CheckUsbAccess(base::Bind(&UsbHostBridge::OnDeviceChecked,
                                      weak_factory_.GetWeakPtr(),
                                      device.get()->guid()));
  }
}

void UsbHostBridge::GetDeviceInfo(const std::string& guid,
                                  GetDeviceInfoCallback callback) {
  device::UsbService* service = GetUsbService();
  if (!service) {
    std::move(callback).Run(nullptr);
    return;
  }
  scoped_refptr<device::UsbDevice> device = service->GetDevice(guid);
  if (!device.get()) {
    LOG(WARNING) << "Unknown USB device " << guid;
    std::move(callback).Run(nullptr);
    return;
  }

  device::mojom::UsbDeviceInfoPtr info =
      device::mojom::UsbDeviceInfo::From(*device);
  // The other side doesn't like optional strings.
  for (const device::mojom::UsbConfigurationInfoPtr& cfg :
       info->configurations) {
    cfg->configuration_name =
        cfg->configuration_name.value_or(base::string16(4, '='));
    for (const device::mojom::UsbInterfaceInfoPtr& iface : cfg->interfaces) {
      for (const device::mojom::UsbAlternateInterfaceInfoPtr& alt :
           iface->alternates) {
        alt->interface_name =
            alt->interface_name.value_or(base::string16(6, '?'));
      }
    }
  }
  std::move(callback).Run(std::move(info));
}

// device::UsbService::Observer callbacks.

// These events are delivered from the thread on which the UsbService object
// was created.
void UsbHostBridge::OnDeviceAdded(scoped_refptr<device::UsbDevice> device) {
  device->CheckUsbAccess(base::Bind(&UsbHostBridge::OnDeviceChecked,
                                    weak_factory_.GetWeakPtr(),
                                    device.get()->guid()));
}

void UsbHostBridge::OnDeviceRemoved(scoped_refptr<device::UsbDevice> device) {
  mojom::UsbHostInstance* usb_host_instance = ARC_GET_INSTANCE_FOR_METHOD(
      arc_bridge_service_->usb_host(), OnDeviceAdded);

  if (!usb_host_instance) {
    VLOG(2) << "UsbInstance not ready yet";
    return;
  }

  usb_host_instance->OnDeviceRemoved(device.get()->guid());
}

// Notifies the observer that the UsbService it depends on is shutting down.
void UsbHostBridge::WillDestroyUsbService() {
  // TODO
}

}  // namespace arc
