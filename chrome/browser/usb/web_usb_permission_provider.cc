// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/usb/web_usb_permission_provider.h"

#include "base/command_line.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/chrome_switches.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/render_frame_host.h"
#include "device/core/device_client.h"

using device::usb::WebUsbDescriptorSet;
using device::usb::WebUsbConfigurationSubsetPtr;
using device::usb::WebUsbFunctionSubsetPtr;

namespace {

bool FindOriginInDescriptorSet(const WebUsbDescriptorSet* set,
                               const GURL& origin,
                               const uint8_t* configuration_value,
                               const uint8_t* interface_number) {
  if (!set)
    return false;
  for (size_t i = 0; i < set->origins.size(); ++i) {
    if (origin.spec() == set->origins[i])
      return true;
  }
  for (size_t i = 0; i < set->configurations.size(); ++i) {
    const device::usb::WebUsbConfigurationSubsetPtr& config =
        set->configurations[i];
    if (configuration_value &&
        *configuration_value != config->configuration_value)
      continue;
    for (size_t j = 0; i < config->origins.size(); ++j) {
      if (origin.spec() == config->origins[j])
        return true;
    }
    for (size_t j = 0; j < config->functions.size(); ++j) {
      const device::usb::WebUsbFunctionSubsetPtr& function =
          config->functions[j];
      // TODO(reillyg): Implement support for Interface Association Descriptors
      // so that this check will match associated interfaces.
      if (interface_number && *interface_number != function->first_interface)
        continue;
      for (size_t k = 0; k < function->origins.size(); ++k) {
        if (origin.spec() == function->origins[k])
          return true;
      }
    }
  }
  return false;
}

bool EnableWebUsbOnAnyOrigin() {
  return base::CommandLine::ForCurrentProcess()->HasSwitch(
      switches::kEnableWebUsbOnAnyOrigin);
}

}  // namespace

// static
void WebUSBPermissionProvider::Create(
    content::RenderFrameHost* render_frame_host,
    mojo::InterfaceRequest<PermissionProvider> request) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  DCHECK(render_frame_host);

  // The created object is owned by its bindings.
  new WebUSBPermissionProvider(render_frame_host, request.Pass());
}

WebUSBPermissionProvider::~WebUSBPermissionProvider() {}

WebUSBPermissionProvider::WebUSBPermissionProvider(
    content::RenderFrameHost* render_frame_host,
    mojo::InterfaceRequest<PermissionProvider> request)
    : render_frame_host_(render_frame_host) {
  bindings_.set_connection_error_handler(base::Bind(
      &WebUSBPermissionProvider::OnConnectionError, base::Unretained(this)));
  bindings_.AddBinding(this, request.Pass());
}

void WebUSBPermissionProvider::HasDevicePermission(
    mojo::Array<device::usb::DeviceInfoPtr> requested_devices,
    const HasDevicePermissionCallback& callback) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  GURL origin = render_frame_host_->GetLastCommittedURL().GetOrigin();

  mojo::Array<mojo::String> allowed_guids(0);
  for (size_t i = 0; i < requested_devices.size(); ++i) {
    const device::usb::DeviceInfoPtr& device = requested_devices[i];
    if (FindOriginInDescriptorSet(device->webusb_allowed_origins.get(), origin,
                                  nullptr, nullptr) &&
        EnableWebUsbOnAnyOrigin())
      allowed_guids.push_back(device->guid);
  }
  callback.Run(allowed_guids.Pass());
}

void WebUSBPermissionProvider::HasConfigurationPermission(
    uint8_t requested_configuration_value,
    device::usb::DeviceInfoPtr device,
    const HasInterfacePermissionCallback& callback) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  callback.Run(FindOriginInDescriptorSet(
      device->webusb_allowed_origins.get(),
      render_frame_host_->GetLastCommittedURL().GetOrigin(),
      &requested_configuration_value, nullptr));
}

void WebUSBPermissionProvider::HasInterfacePermission(
    uint8_t requested_interface,
    uint8_t configuration_value,
    device::usb::DeviceInfoPtr device,
    const HasInterfacePermissionCallback& callback) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  callback.Run(FindOriginInDescriptorSet(
      device->webusb_allowed_origins.get(),
      render_frame_host_->GetLastCommittedURL().GetOrigin(),
      &configuration_value, &requested_interface));
}

void WebUSBPermissionProvider::Bind(
    mojo::InterfaceRequest<device::usb::PermissionProvider> request) {
  bindings_.AddBinding(this, request.Pass());
}

void WebUSBPermissionProvider::OnConnectionError() {
  if (bindings_.empty())
    delete this;
}
