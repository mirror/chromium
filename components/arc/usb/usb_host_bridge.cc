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
}

void UsbHostBridge::OnDeviceOpened(const OpenDeviceCallback& callback,
                                   base::ScopedFD fd) {
  DCHECK(fd.is_valid());
  mojo::edk::ScopedPlatformHandle platform_handle{
      mojo::edk::PlatformHandle(fd.release())};
  MojoHandle wrapped_handle;
  MojoResult wrap_result = mojo::edk::CreatePlatformHandleWrapper(
      std::move(platform_handle), &wrapped_handle);
  if (wrap_result != MOJO_RESULT_OK) {
    LOG(ERROR) << "Failed to wrap device FD. Closing: " << wrap_result;
    callback.Run(mojo::ScopedHandle());
    return;
  }
  mojo::ScopedHandle scoped_handle{mojo::Handle(wrapped_handle)};
  callback.Run(std::move(scoped_handle));
}

void UsbHostBridge::OnDeviceOpenError(const OpenDeviceCallback& callback,
                                      const std::string& error_name,
                                      const std::string& error_message) {
  LOG(WARNING) << "Cannot open USB device: " << error_name << ": "
               << error_message;
  callback.Run(mojo::ScopedHandle());
}

void UsbHostBridge::OpenDevice(const std::string& guid,
                               const OpenDeviceCallback& callback) {
  device::UsbService* service = GetUsbService();
  if (!service) {
    callback.Run(mojo::ScopedHandle());
    return;
  }

  device::UsbDeviceLinux* device =
      static_cast<device::UsbDeviceLinux*>(service->GetDevice(guid).get());
  if (!device) {
    callback.Run(mojo::ScopedHandle());
    return;
  }

  // TODO trigger permission request

  chromeos::PermissionBrokerClient* client =
      chromeos::DBusThreadManager::Get()->GetPermissionBrokerClient();
  DCHECK(client) << "Could not get permission broker client.";
  client->OpenPath(device->device_path(),
                   base::Bind(&UsbHostBridge::OnDeviceOpened,
                              weak_factory_.GetWeakPtr(), callback),
                   base::Bind(&UsbHostBridge::OnDeviceOpenError,
                              weak_factory_.GetWeakPtr(), callback));
}

void UsbHostBridge::GetDevicesList(const GetDevicesListCallback& callback) {
  device::UsbService* service = GetUsbService();
  if (!service) {
    callback.Run(std::vector<std::string>());
    return;
  }

  service->GetDevices(base::Bind(&UsbHostBridge::OnGetDevicesComplete,
                                 weak_factory_.GetWeakPtr(), callback));
}

void UsbHostBridge::OnGetDevicesComplete(
    const GetDevicesListCallback& callback,
    const std::vector<scoped_refptr<device::UsbDevice>>& devices) {
  std::vector<std::string> ids;
  for (const scoped_refptr<device::UsbDevice>& device : devices) {
    // TODO: filter only authorized devices

    ids.push_back(std::move(device.get()->guid()));
  }

  callback.Run(std::move(ids));
}
void UsbHostBridge::GetDeviceInfo(const std::string& guid,
                                  const GetDeviceInfoCallback& callback) {
  device::UsbService* service = GetUsbService();
  if (!service) {
    callback.Run(nullptr);
    return;
  }
  scoped_refptr<device::UsbDevice> device = service->GetDevice(guid);
  if (!device.get()) {
    LOG(WARNING) << "Unknown USB device " << guid;
    callback.Run(nullptr);
    return;
  }

  callback.Run(mojo::ConvertTo<device::mojom::UsbDeviceInfoPtr>(*device));
}

// device::UsbService::Observer callbacks.

// These events are delivered from the thread on which the UsbService object
// was created.
void UsbHostBridge::OnDeviceAdded(scoped_refptr<device::UsbDevice> device) {
  mojom::UsbHostInstance* usb_host_instance = ARC_GET_INSTANCE_FOR_METHOD(
      arc_bridge_service_->usb_host(), OnDeviceAdded);

  if (!usb_host_instance) {
    VLOG(2) << "UsbInstance not ready yet";
    return;
  }

  usb_host_instance->OnDeviceAdded(device.get()->guid());
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
