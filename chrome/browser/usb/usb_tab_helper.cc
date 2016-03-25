// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/usb/usb_tab_helper.h"

#include <utility>

#include "base/memory/scoped_ptr.h"
#include "chrome/browser/usb/web_usb_permission_provider.h"
#include "device/usb/mojo/device_manager_impl.h"

#if defined(OS_ANDROID)
#include "chrome/browser/android/usb/web_usb_chooser_service_android.h"
#else
#include "chrome/browser/usb/web_usb_chooser_service.h"
#endif  // defined(OS_ANDROID)

using content::RenderFrameHost;
using content::WebContents;

DEFINE_WEB_CONTENTS_USER_DATA_KEY(UsbTabHelper);

struct FrameUsbServices {
  scoped_ptr<WebUSBPermissionProvider> permission_provider;
#if defined(OS_ANDROID)
  scoped_ptr<WebUsbChooserServiceAndroid> chooser_service;
#else
  scoped_ptr<WebUsbChooserService> chooser_service;
#endif  // defined(OS_ANDROID)
};

// static
UsbTabHelper* UsbTabHelper::GetOrCreateForWebContents(
    WebContents* web_contents) {
  UsbTabHelper* tab_helper = FromWebContents(web_contents);
  if (!tab_helper) {
    CreateForWebContents(web_contents);
    tab_helper = FromWebContents(web_contents);
  }
  return tab_helper;
}

UsbTabHelper::~UsbTabHelper() {}

void UsbTabHelper::CreateDeviceManager(
    RenderFrameHost* render_frame_host,
    mojo::InterfaceRequest<device::usb::DeviceManager> request) {
  DCHECK(WebContents::FromRenderFrameHost(render_frame_host) == web_contents());
  device::usb::DeviceManagerImpl::Create(
      GetPermissionProvider(render_frame_host), std::move(request));
}

void UsbTabHelper::CreateChooserService(
    content::RenderFrameHost* render_frame_host,
    mojo::InterfaceRequest<device::usb::ChooserService> request) {
  GetChooserService(render_frame_host, std::move(request));
}

UsbTabHelper::UsbTabHelper(WebContents* web_contents)
    : content::WebContentsObserver(web_contents) {}

void UsbTabHelper::RenderFrameDeleted(RenderFrameHost* render_frame_host) {
  frame_usb_services_.erase(render_frame_host);
}

FrameUsbServices* UsbTabHelper::GetFrameUsbService(
    content::RenderFrameHost* render_frame_host) {
  FrameUsbServicesMap::const_iterator it =
      frame_usb_services_.find(render_frame_host);
  if (it == frame_usb_services_.end()) {
    scoped_ptr<FrameUsbServices> frame_usb_services(new FrameUsbServices());
    it = (frame_usb_services_.insert(
              std::make_pair(render_frame_host, std::move(frame_usb_services))))
             .first;
  }
  return it->second.get();
}

base::WeakPtr<device::usb::PermissionProvider>
UsbTabHelper::GetPermissionProvider(RenderFrameHost* render_frame_host) {
  FrameUsbServices* frame_usb_services = GetFrameUsbService(render_frame_host);
  if (!frame_usb_services->permission_provider) {
    frame_usb_services->permission_provider.reset(
        new WebUSBPermissionProvider(render_frame_host));
  }
  return frame_usb_services->permission_provider->GetWeakPtr();
}

void UsbTabHelper::GetChooserService(
    content::RenderFrameHost* render_frame_host,
    mojo::InterfaceRequest<device::usb::ChooserService> request) {
  FrameUsbServices* frame_usb_services = GetFrameUsbService(render_frame_host);
  if (!frame_usb_services->chooser_service) {
    frame_usb_services->chooser_service.reset(
#if defined(OS_ANDROID)
        new WebUsbChooserServiceAndroid(render_frame_host));
#else
        new WebUsbChooserService(render_frame_host));
#endif  // defined(OS_ANDROID)
  }
  frame_usb_services->chooser_service->Bind(std::move(request));
}
