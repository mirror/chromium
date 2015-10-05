// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_USB_WEB_USB_CLIENT_IMPL_H_
#define CONTENT_RENDERER_USB_WEB_USB_CLIENT_IMPL_H_

#include "base/macros.h"
#include "device/devices_app/usb/public/interfaces/device_manager.mojom.h"
#include "third_party/WebKit/public/platform/modules/webusb/WebUSBClient.h"

namespace content {

class ServiceRegistry;

class WebUSBClientImpl : public blink::WebUSBClient {
 public:
  explicit WebUSBClientImpl(content::ServiceRegistry* service_registry);
  ~WebUSBClientImpl() override;

 private:
  // blink::WebUSBClient implementation:
  void getDevices(blink::WebUSBClientGetDevicesCallbacks* callbacks) override;
  void requestDevice(
      const blink::WebUSBDeviceRequestOptions& options,
      blink::WebUSBClientRequestDeviceCallbacks* callbacks) override;
  void setObserver(Observer* observer) override;

  void OnDeviceChangeNotification(
      device::usb::DeviceChangeNotificationPtr notification);

  device::usb::DeviceManagerPtr device_manager_;
  Observer* observer_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(WebUSBClientImpl);
};

}  // namespace content

#endif  // CONTENT_RENDERER_USB_WEB_USB_CLIENT_IMPL_H_
