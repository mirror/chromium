// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/arc/video/gpu_arc_protected_buffer_service_host.h"

#include <memory>
#include <string>

#include "base/location.h"
#include "base/logging.h"
#include "base/memory/singleton.h"
#include "base/threading/thread_checker.h"
#include "components/arc/arc_bridge_service.h"
#include "components/arc/arc_browser_context_keyed_service_factory_base.h"
#include "components/arc/common/video_decode_accelerator.mojom.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/gpu_service_registry.h"
#include "content/public/common/service_manager_connection.h"
#include "mojo/edk/embedder/outgoing_broker_client_invitation.h"
#include "mojo/edk/embedder/platform_channel_pair.h"
#include "services/service_manager/public/cpp/connector.h"
#include "services/ui/public/interfaces/arc.mojom.h"
#include "services/ui/public/interfaces/constants.mojom.h"
#include "ui/base/ui_base_switches_util.h"

namespace arc {

namespace {

class GpuArcProtectedBufferServiceHostFactory
    : public internal::ArcBrowserContextKeyedServiceFactoryBase<
          GpuArcProtectedBufferServiceHost,
          GpuArcProtectedBufferServiceHostFactory> {
 public:
  static constexpr const char* kName =
      "GpuArcProtectedBufferServiceHostFactory";
  static GpuArcProtectedBufferHostFactory* GetInstance() {
    return base::Singleton<GpuArcProtectedBufferServiceHostFactory>::get();
  }

 private:
  friend base::DefaultSingletonTraits<GpuArcProtectedBufferHostFactory>;
  GpuArcProtectedBufferHostFactory() = default;
  ~GpuArcProtectedBufferHostFactory() override = default;
};

class ProtectedBufferFactoryService : public mojom::ProtectedBufferFactory {
 public:
  // TODO(hiroh): Viz
  ProtectedBufferFactoryService() = default;
  ~ProtectedBufferFactoryService() override = default;

  void CreateProtectedBufferAllocator(
      mojom::ProtectedBufferAllocatorRequest request) override {
    content::BrowserThread::PostTask(
        content::BrowserThread::IO, FROM_HERE,
        base::BindOnce(&content::BindInterfaceInGpuProcess<
                           mojom::ProtectedBufferAllocator>,
                       std::move(request)));
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(ProtectedBufferFactoryService);
};

//  TODO(hiroh): Viz

std::unique_ptr<mojom::ProtectedBufferFactory>
CreateProtectedBufferAcceleratorFactory() {
  return std::make_unique<GProtectedBufferFactoryService>();
}
}  // namespace

// static
GpuArcProtectedBufferServiceHost*
GpuArcProtectedBufferServiceHost::GetForBrowserContext(
    content::BrowserContext* context) {
  return GpuArcProtectedBufferServiceHostFactory::GetForBrowserContext(context);
}

GpuArcProtectedBufferServiceHost::GpuArcProtectedBufferServiceHost(
    content::BrowserContext* context,
    ArcBridgeService* bridge_service)
    : arc_bridge_service_(bridge_service),
      protected_buffer_factory_(CreateProtectedBufferAcceleratorFactory()) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  arc_bridge_service_->protected_buffer()->SetHost(this);
}

GpuArcProtectedBufferServiceHost::~GpuArcProtectedBufferServiceHost() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  arc_bridge_service_->protected_buffer()->SetHost(nullptr);
}

void GpuArcProtectedBufferServiceHost::OnBootstrapProtectedBufferFactory(
    OnBootstrapProtectedBufferAcceleratorFactoryCallback callback) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  // Hardcode pid 0 since it is unused in mojo.
  const base::ProcessHandle kUnusedChildProcessHandle =
      base::kNullProcessHandle;
  mojo::edk::OutgoingBrokerClientInvitation invitation;
  mojo::edk::PlatformChannelPair channel_pair;
  std::string token = mojo::edk::GenerateRandomToken();
  mojo::ScopedMessagePipeHandle server_pipe =
      invitation.AttachMessagePipe(token);
  invitation.Send(
      kUnusedChildProcessHandle,
      mojo::edk::ConnectionParams(mojo::edk::TransportProtocol::kLegacy,
                                  channel_pair.PassServerHandle()));

  MojoHandle wrapped_handle;
  MojoResult wrap_result = mojo::edk::CreatePlatformHandleWrapper(
      channel_pair.PassClientHandle(), &wrapped_handle);
  if (wrap_result != MOJO_RESULT_OK) {
    LOG(ERROR) << "Pipe failed to wrap handles. Closing: " << wrap_result;
    std::move(callback).Run(mojo::ScopedHandle(), std::string());
    return;
  }
  mojo::ScopedHandle child_handle{mojo::Handle(wrapped_handle)};

  std::move(callback).Run(std::move(child_handle), token);

  // The binding will be removed automatically, when the binding is destroyed.
  protected_buffer_factory_bindings_.AddBinding(
      protected_buffer_factory_.get(),
      mojom::ProtectedBufferFactoryRequest(std::move(server_pipe)));
}

}  // namespace arc
