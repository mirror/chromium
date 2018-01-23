// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_VIDEO_CAPTURE_EXAMPLE_PLUGIN_CONNECTION_DISPATCHER_H_
#define SERVICES_VIDEO_CAPTURE_EXAMPLE_PLUGIN_CONNECTION_DISPATCHER_H_

#include "base/threading/thread.h"
#include "mojo/edk/embedder/scoped_platform_handle.h"
#include "mojo/public/cpp/bindings/binding_set.h"
#include "services/video_capture/public/interfaces/plugin.mojom.h"

namespace video_capture {
namespace example_plugin {

// Listens for incoming connections from the Chromium video capture service.
// Invokes |device_factory_registered_cb| when a connection has been established
// successfully.
class ConnectionDispatcher
    : public video_capture::mojom::PluginConnectionDispatcher {
 public:
  using DeviceFactoryRegisteredCallback =
      base::RepeatingCallback<void(video_capture::mojom::DeviceFactoryPtr)>;

  ConnectionDispatcher(
      DeviceFactoryRegisteredCallback device_factory_registered_cb);
  ~ConnectionDispatcher() override;

  void StartListeningForIncomingConnection();

  // video_capture::mojom::PluginConnectionDispatcher implementation.
  void RegisterDeviceFactory(
      video_capture::mojom::DeviceFactoryPtr device_factory) override;

 private:
  std::unique_ptr<base::Thread> ipc_thread_;
  std::unique_ptr<mojo::edk::ScopedIPCSupport> ipc_support_;

  mojo::BindingSet<video_capture::mojom::PluginConnectionDispatcher>
      binding_set_;
  DeviceFactoryRegisteredCallback device_factory_registered_cb_;
};

}  // namespace example_plugin
}  // namespace video_capture

#endif  // SERVICES_VIDEO_CAPTURE_EXAMPLE_PLUGIN_CONNECTION_DISPATCHER_H_
