// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_EXTERNAL_PROCESS_CONNECTION_DISPATCHER_H_
#define CONTENT_BROWSER_EXTERNAL_PROCESS_CONNECTION_DISPATCHER_H_

#include "base/sequenced_task_runner.h"
#include "base/threading/thread.h"
#include "mojo/edk/embedder/scoped_platform_handle.h"
#include "mojo/public/cpp/bindings/binding_set.h"
#include "services/video_capture/public/interfaces/device_factory_provider.mojom.h"

namespace content {

// Listens for incoming connections from external processes and connects them
// to this instance via the Mojo interface
// video_capture::mojom::DeviceFactoryProvider.
class ExternalProcessConnectionDispatcher
    : public video_capture::mojom::DeviceFactoryProvider {
 public:
  ExternalProcessConnectionDispatcher();
  ~ExternalProcessConnectionDispatcher() override;

  void StartListeningForIncomingConnection();

  // video_capture::mojom::DeviceFactoryProvider implementation.
  void ConnectToDeviceFactory(
      video_capture::mojom::DeviceFactoryRequest request) override;
  void SetShutdownDelayInSeconds(float seconds) override;

 private:
  void ListenAndBlockForIncomingConnection();
  void SendMojoInvitationToExternalProcess(
      mojo::edk::ScopedPlatformHandle handle);
  void OnExternalProcessDisconnected();

  std::unique_ptr<base::Thread> thread_;
  scoped_refptr<base::SequencedTaskRunner> owner_task_runner_;
  mojo::Binding<video_capture::mojom::DeviceFactoryProvider> binding_;
};

}  // namespace content

#endif  // CONTENT_BROWSER_EXTERNAL_PROCESS_CONNECTION_DISPATCHER_H_
