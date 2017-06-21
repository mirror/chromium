// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_CAPTURE_VIDEO_CHROMEOS_CAMERA_HAL_DISPATCHER_IMPL_H_
#define MEDIA_CAPTURE_VIDEO_CHROMEOS_CAMERA_HAL_DISPATCHER_IMPL_H_

#include <memory>
#include <set>

#include "base/memory/singleton.h"
#include "base/threading/thread.h"
#include "media/capture/capture_export.h"
#include "media/capture/video/chromeos/mojo/arc_camera3_service.mojom.h"
#include "mojo/edk/embedder/scoped_platform_handle.h"
#include "mojo/public/cpp/bindings/binding_set.h"
#include "mojo/public/cpp/bindings/interface_ptr_set.h"

namespace base {

class SingleThreadTaskRunner;
class WaitableEvent;

}  // namespace base

namespace media {

class CAPTURE_EXPORT CameraClientObserver {
 public:
  virtual void OnChannelCreated(arc::mojom::CameraModulePtr camera_module) = 0;
};

// The ARC++ camera HAL v3 Mojo dispatcher.  The dispatcher acts as a proxy and
// waits for the server (e.g. the camera HAL process) and the clients (e.g. the
// CameraHalDelegate or the HAL v3 shim in Android cameraserver process) to
// register.  There can only be one server registered, with multiple clients
// requesting connections to the server.  For each client, the dispatcher is
// responsible for creating a Mojo channel to the server and pass the
// established Mojo channel to the client in order to set up a Mojo channel
// between the client and the server.
//
// The CameraHalDispatcherImpl is designed to help the server and the clients to
// recover from errors easily.  For example, when the camera HAL process crashes
// the CameraHalDispatcherImpl still holds the connections of the clients. When
// the camera HAL reconnects the CameraHalDispatcherImpl can then quickly
// restore the Mojo channels between the clients and the camera HAL process in
// RegiserServer().
class CAPTURE_EXPORT CameraHalDispatcherImpl final
    : public arc::mojom::CameraHalDispatcher {
 public:
  static CameraHalDispatcherImpl* GetInstance();

  bool Start();

  void AddClientObserver(std::unique_ptr<CameraClientObserver> observer);

  bool IsStarted();

 private:
  friend base::DefaultSingletonTraits<CameraHalDispatcherImpl>;

  CameraHalDispatcherImpl();
  ~CameraHalDispatcherImpl() override;

  // Creates the unix domain socket for the camera client processes and the
  // camera HALv3 adapter process to connect.
  void CreateSocket(base::WaitableEvent* started);

  // Waits for incoming connections (from HAL process or from client processes).
  // Runs on |blocking_io_thread_|.
  void StartServiceLoop(mojo::edk::ScopedPlatformHandle socket_fd,
                        base::WaitableEvent* started);

  // CameraHalDispatcherImpl implementations.
  void RegisterServer(arc::mojom::CameraHalServerPtr server) final;
  void RegisterClient(arc::mojom::CameraHalClientPtr client) final;

  void AddClientObserverOnProxyThread(
      std::unique_ptr<CameraClientObserver> observer);

  void EstablishMojoChannel(CameraClientObserver* client_observer);

  // Handler for incoming Mojo connection on the unix domain socket.
  void OnPeerConnected(mojo::ScopedMessagePipeHandle message_pipe);

  // Mojo connection error handlers.
  void OnCameraHalServerConnectionError();
  void OnCameraHalClientConnectionError(CameraClientObserver* client);

  void StopOnProxyThread();

  mojo::edk::ScopedPlatformHandle proxy_fd_;
  base::ScopedFD cancel_pipe_;

  base::Thread proxy_thread_;
  base::Thread blocking_io_thread_;
  scoped_refptr<base::SingleThreadTaskRunner> proxy_task_runner_;
  scoped_refptr<base::SingleThreadTaskRunner> blocking_io_task_runner_;

  mojo::BindingSet<arc::mojom::CameraHalDispatcher> binding_set_;

  arc::mojom::CameraHalServerPtr camera_hal_server_;

  std::set<std::unique_ptr<CameraClientObserver>> client_observers_;

  DISALLOW_COPY_AND_ASSIGN(CameraHalDispatcherImpl);
};

}  // namespace media

#endif  // MEDIA_CAPTURE_VIDEO_CHROMEOS_ARC_CAMERA3_SERVICE_H_
