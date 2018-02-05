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
  static GpuArcProtectedBufferServiceHostFactory* GetInstance() {
    return base::Singleton<GpuArcProtectedBufferServiceHostFactory>::get();
  }

 private:
  friend base::DefaultSingletonTraits<GpuArcProtectedBufferServiceHostFactory>;
  GpuArcProtectedBufferServiceHostFactory() = default;
  ~GpuArcProtectedBufferServiceHostFactory() override = default;
};

//  TODO(hiroh): Implement for Viz.
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
    : arc_bridge_service_(bridge_service) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  arc_bridge_service_->protected_buffer()->SetHost(this);
}

GpuArcProtectedBufferServiceHost::~GpuArcProtectedBufferServiceHost() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  arc_bridge_service_->protected_buffer()->SetHost(nullptr);
}

void GpuArcProtectedBufferServiceHost::CreateProtectedBufferAllocator(
    mojom::ProtectedBufferAllocatorRequest request) {
  content::BrowserThread::PostTask(
      content::BrowserThread::IO, FROM_HERE,
      base::BindOnce(
          &content::BindInterfaceInGpuProcess<mojom::ProtectedBufferAllocator>,
          std::move(request)));
}
}  // namespace arc
