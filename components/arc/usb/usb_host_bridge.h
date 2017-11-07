// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_ARC_USB_USB_HOST_BRIDGE_H_
#define COMPONENTS_ARC_USB_USB_HOST_BRIDGE_H_

#include <stdint.h>

#include <string>
#include <vector>

#include "base/callback.h"
#include "base/files/scoped_file.h"
#include "base/macros.h"
#include "base/scoped_observer.h"
#include "components/arc/common/usb_host.mojom.h"
#include "components/arc/instance_holder.h"
#include "components/keyed_service/core/keyed_service.h"
#include "device/usb/public/interfaces/device_manager.mojom.h"
#include "device/usb/usb_device.h"
#include "device/usb/usb_service.h"
#include "mojo/public/cpp/bindings/binding.h"

namespace content {
class BrowserContext;
}  // namespace content

namespace arc {

class ArcBridgeService;

// Private implementation of UsbHost.
class UsbHostBridge : public KeyedService,
                      public InstanceHolder<mojom::UsbHostInstance>::Observer,
                      public device::UsbService::Observer,
                      public mojom::UsbHost {
 public:
  // Returns singleton instance for the given BrowserContext,
  // or nullptr if the browser |context| is not allowed to use ARC.
  static UsbHostBridge* GetForBrowserContext(content::BrowserContext* context);

  // The constructor will register an Observer with ArcBridgeService.
  explicit UsbHostBridge(content::BrowserContext* context,
                         ArcBridgeService* bridge_service);
  ~UsbHostBridge() override;

  // ARC -> Chrome calls (Mojo host interface):

  void RequestPermission(const std::string& guid,
                         const std::string& package,
                         bool interactive,
                         RequestPermissionCallback callback) override;

  void OpenDevice(const std::string& guid,
                  OpenDeviceCallback callback) override;

  void GetDeviceInfo(const std::string& guid,
                     GetDeviceInfoCallback callback) override;

  // Overriden from device::UsbService::Observer.
  void OnDeviceAdded(scoped_refptr<device::UsbDevice> device) override;
  void OnDeviceRemoved(scoped_refptr<device::UsbDevice> device) override;
  void WillDestroyUsbService() override;

  // Overridden from InstanceHolder<mojom::UsbHostInstance>::Observer:
  void OnInstanceReady() override;

 private:
  ArcBridgeService* const arc_bridge_service_;  // Owned by ArcServiceManager.
  mojo::Binding<arc::mojom::UsbHost> binding_;
  mojom::UsbHostPtr usb_host_ptr_;

  void OnDeviceOpened(OpenDeviceCallback callback, base::ScopedFD fd);
  void OnDeviceOpenError(OpenDeviceCallback callback,
                         const std::string& error_name,
                         const std::string& error_message);
  void OnDeviceChecked(const std::string& guid, bool allowed);
  void OnGetDevicesComplete(
      const std::vector<scoped_refptr<device::UsbDevice>>& devices);

  void DoRequestUserAuthorization(const std::string& guid,
                                  const std::string& package,
                                  RequestPermissionCallback callback);
  bool HasPermissionForDevice(const std::string& guid);

  ScopedObserver<device::UsbService, device::UsbService::Observer>
      usb_observer_;

  // WeakPtrFactory to use for callbacks.
  base::WeakPtrFactory<UsbHostBridge> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(UsbHostBridge);
};

}  // namespace arc

#endif  // COMPONENTS_ARC_USB_USB_HOST_BRIDGE_H_
