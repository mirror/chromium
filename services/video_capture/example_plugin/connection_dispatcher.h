// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_VIDEO_CAPTURE_EXAMPLE_PLUGIN_CONNECTION_DISPATCHER_H_
#define SERVICES_VIDEO_CAPTURE_EXAMPLE_PLUGIN_CONNECTION_DISPATCHER_H_

#include "base/threading/thread.h"
#include "mojo/edk/embedder/scoped_platform_handle.h"
#include "mojo/public/cpp/bindings/binding_set.h"
#include "services/video_capture/public/interfaces/device_factory_provider.mojom.h"

namespace video_capture {
namespace example_plugin {

// Attempts to establish a Mojo connection to a Chromium instance running on the
// local machine. Repeats attempts at regular intervals.
// Invokes |device_factory_registered_cb| when a connection has been established
// successfully.
class ConnectionDispatcher {
 public:
  using ConnectionEstablishedCallback =
      base::RepeatingCallback<void(video_capture::mojom::DeviceFactoryPtr)>;

  ConnectionDispatcher(ConnectionEstablishedCallback connection_established_cb);
  ~ConnectionDispatcher();

  void RunConnectionAttemptLoop();

 private:
  std::unique_ptr<base::Thread> ipc_thread_;
  std::unique_ptr<mojo::edk::ScopedIPCSupport> ipc_support_;

  ConnectionEstablishedCallback connection_established_cb_;
  mojom::DeviceFactoryProviderPtr provider_;
};

}  // namespace example_plugin
}  // namespace video_capture

#endif  // SERVICES_VIDEO_CAPTURE_EXAMPLE_PLUGIN_CONNECTION_DISPATCHER_H_
