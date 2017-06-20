// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_CAPTURE_VIDEO_CHROMEOS_CAMERA_HAL_DISPATCHER_IMPL_H_
#define MEDIA_CAPTURE_VIDEO_CHROMEOS_CAMERA_HAL_DISPATCHER_IMPL_H_

#include <memory>
#include <set>

#include "base/memory/singleton.h"
#include "base/memory/weak_ptr.h"
#include "base/single_thread_task_runner.h"
#include "base/threading/thread.h"
#include "media/capture/capture_export.h"
#include "media/capture/video/chromeos/mojo/arc_camera3_service.mojom.h"
#include "mojo/edk/embedder/scoped_platform_handle.h"
#include "mojo/public/cpp/bindings/binding_set.h"
#include "mojo/public/cpp/bindings/interface_ptr_set.h"

namespace media {

class CAPTURE_EXPORT CameraClientObserver
    : public base::SupportsWeakPtr<CameraClientObserver> {
 public:
  virtual void SetUpChannel(arc::mojom::CameraModulePtr camera_module) = 0;
};

class CAPTURE_EXPORT CameraHalDispatcherImpl final
    : public arc::mojom::CameraHalDispatcher {
 public:
  static CameraHalDispatcherImpl* GetInstance();

  bool Start();

  void AddClientObserver(std::unique_ptr<CameraClientObserver> observer);

  bool is_started() {
    return proxy_thread_.IsRunning() && blocking_io_thread_.IsRunning();
  }

 private:
  friend base::DefaultSingletonTraits<CameraHalDispatcherImpl>;

  CameraHalDispatcherImpl();
  ~CameraHalDispatcherImpl() override;

  // Creates the unix domain socket for the camera client processes and the
  // camera HALv3 adapter process to connect.
  void CreateSocket();

  void CreateServiceLoop(mojo::edk::ScopedPlatformHandle socket_fd);

  // Waits for incoming connections (from HAL process or from client processes).
  // Runs on |blocking_io_thread_|.
  void WaitForIncomingConnection(base::ScopedFD cancel_fd);

  // CameraHalDispatcherImpl implementations.
  void RegisterServer(arc::mojom::CameraHalServerPtr server) final;
  void RegisterClient(arc::mojom::CameraHalClientPtr client) final;

  void AddClientObserverOnProxyThread(
      std::unique_ptr<CameraClientObserver> observer);

  void EstablishMojoChannel(CameraClientObserver* client_observer);

  // Handler for incoming Mojo connection on the unix domain socket.
  void OnPeerConnected(mojo::ScopedMessagePipeHandle message_pipe);

  // Mojo connection error handlers.
  void OnPeerConnectionError(
      mojo::Binding<arc::mojom::CameraHalDispatcher>* binding);
  void OnCameraHalServerConnectionError();
  void OnCameraHalClientConnectionError(CameraClientObserver* client);

  void StopOnProxyThread();

  mojo::edk::ScopedPlatformHandle proxy_fd_;
  base::ScopedFD cancel_pipe_;

  base::Thread proxy_thread_;
  base::Thread blocking_io_thread_;
  scoped_refptr<base::SingleThreadTaskRunner> proxy_task_runner_;
  scoped_refptr<base::SingleThreadTaskRunner> blocking_io_task_runner_;

  std::set<std::unique_ptr<mojo::Binding<arc::mojom::CameraHalDispatcher>>>
      bindings_;

  arc::mojom::CameraHalServerPtr camera_hal_server_;

  std::set<std::unique_ptr<CameraClientObserver>> client_observers_;

  DISALLOW_COPY_AND_ASSIGN(CameraHalDispatcherImpl);
};

}  // namespace media

#endif  // MEDIA_CAPTURE_VIDEO_CHROMEOS_ARC_CAMERA3_SERVICE_H_
