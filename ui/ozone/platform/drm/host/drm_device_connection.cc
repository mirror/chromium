// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// TODO(rjk): rename me to drm_device_connector
#include "ui/ozone/platform/drm/host/drm_device_connection.h"

#include "mojo/public/cpp/bindings/interface_request.h"
#include "mojo/public/cpp/system/message_pipe.h"
#include "ui/ozone/public/gpu_platform_support_host.h"
#include "ui/ozone/platform/drm/host/host_drm_device.h"
#include "services/service_manager/public/cpp/connector.h"
#include "services/ui/public/interfaces/constants.mojom.h"

// TODO(rjkroege): In the future when we CrOS is always on mojo, we can remove this utility code.
namespace {
using BinderCallback = ui::GpuPlatformSupportHost::GpuHostBindInterfaceCallback;

void BindInterfaceInGpuProcess(const std::string& interface_name,
                               mojo::ScopedMessagePipeHandle interface_pipe,  
	const BinderCallback& binder_callback
) {
  return binder_callback.Run(interface_name, std::move(interface_pipe));
}

template <typename Interface>
void BindInterfaceInGpuProcess(mojo::InterfaceRequest<Interface> request,
	const BinderCallback& binder_callback
) {
  BindInterfaceInGpuProcess(Interface::Name_,
                            std::move(request.PassMessagePipe()), binder_callback);
}

}  // namespace

namespace ui {

DrmDeviceConnector::DrmDeviceConnector(service_manager::Connector* connector, base::WeakPtr<HostDrmDevice> host_drm_device)  :  connector_(connector), host_drm_device_(host_drm_device)
{ 
}

DrmDeviceConnector::~DrmDeviceConnector() {
}


 void DrmDeviceConnector::OnGpuProcessLaunched(
      int host_id,
      scoped_refptr<base::SingleThreadTaskRunner> ui_runner,
      scoped_refptr<base::SingleThreadTaskRunner> send_runner,
      const base::Callback<void(IPC::Message*)>& send_callback) {
	
	NOTREACHED() << "shouldn't get here";

}


  void DrmDeviceConnector::OnChannelDestroyed(int host_id)  {
	// Must do something here to handle restart. 
	 NOTIMPLEMENTED() << "need to so something here to handle Viz restart\n";
}

void DrmDeviceConnector::OnGpuServiceLaunched(GpuHostBindInterfaceCallback binder) {
	// We need to preserve this to let us bind the cursor interfaces later.
	binder_callback_ = std::move(binder);
	if (host_drm_device_) {
		// TODO(rjk): Change the name of this function.
		host_drm_device_->OnGpuServiceLaunched(*this);
	}
}



bool DrmDeviceConnector::OnMessageReceived(const IPC::Message& message) {
	 NOTREACHED() << "This class should only be used with mojo transport but here we're wrongly getting invoked to handle IPC communication.";
	return false;
}


void DrmDeviceConnector::BindInterface(ui::ozone::mojom::DrmDevicePtr* drm_device_ptr) const {
	if (connector_) {

		LOG(ERROR) << "@@@ DrmDeviceConnector::BindInterface have connector_";

		connector_->BindInterface(ui::mojom::kServiceName, drm_device_ptr);
	} else {
		LOG(ERROR) << "@@@ DrmDeviceConnector::BindInterface no have connector_, ask BindIn...";
		auto request = mojo::MakeRequest(drm_device_ptr);
		BindInterfaceInGpuProcess(std::move(request), binder_callback_);

	}
}


} // namespace ui