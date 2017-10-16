// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/embedded_ui_service_info_factory.h"

#include "base/message_loop/message_loop.h"
#include "build/build_config.h"
#include "content/public/browser/discardable_shared_memory_manager.h"
#include "services/ui/public/interfaces/constants.mojom.h"
#include "services/ui/service.h"

#if defined(OS_CHROMEOS)
#include "chrome/gpu/gpu_arc_video_decode_accelerator.h"
#include "chrome/gpu/gpu_arc_video_encode_accelerator.h"
#include "components/arc/common/video_decode_accelerator.mojom.h"
#include "components/arc/common/video_encode_accelerator.mojom.h"
#include "content/public/common/service_manager_connection.h"
#include "media/gpu/features.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "services/service_manager/public/cpp/binder_registry.h"

#if BUILDFLAG(USE_VAAPI)
#include "media/gpu/vaapi_wrapper.h"
#endif

#endif

namespace {

#if defined(OS_CHROMEOS)
// TODO(penghuang): Remove this class when we have a separate viz process.
// https://crbug.com/774514
class ForwardingUIService : public service_manager::ForwardingService {
 public:
  explicit ForwardingUIService(const ui::Service::InProcessConfig* config)
      : service_manager::ForwardingService(&ui_service_), ui_service_(config) {}
  ~ForwardingUIService() override {}

 private:
  void OnStart() override {
#if BUILDFLAG(USE_VAAPI)
    // Initialize media codec. The UI service is running in a priviliged
    // process. We don't need care when to initialize media codec. When we have
    // a separate GPU (or VIZ) service process, we should initialize them
    // before sandboxing.
    media::VaapiWrapper::PreSandboxInitialization();
#endif

    // Make ForwardingUIService provides below interfaces. When the GPU (or VIZ)
    // is separated from UI service, the GPU service should provide those
    // interfaces.
    registry_.AddInterface(
        base::Bind(&ForwardingUIService::BindRequest<
                       arc::mojom::VideoDecodeAccelerator,
                       chromeos::arc::GpuArcVideoDecodeAccelerator>,
                   base::Unretained(this)));
    registry_.AddInterface(
        base::Bind(&ForwardingUIService::BindRequest<
                       arc::mojom::VideoEncodeAccelerator,
                       chromeos::arc::GpuArcVideoEncodeAccelerator>,
                   base::Unretained(this)));
    service_manager::ForwardingService::OnStart();
  }

  void OnBindInterface(const service_manager::BindSourceInfo& source_info,
                       const std::string& interface_name,
                       mojo::ScopedMessagePipeHandle interface_pipe) override {
    if (registry_.TryBindInterface(interface_name, &interface_pipe))
      return;
    service_manager::ForwardingService::OnBindInterface(
        source_info, interface_name, std::move(interface_pipe));
  }

  bool OnServiceManagerConnectionLost() override {
    return service_manager::ForwardingService::OnServiceManagerConnectionLost();
  }

  template <typename Interface, typename Accelerator>
  void BindRequest(mojo::InterfaceRequest<Interface> request) {
    ui_service_.GetGpuThreadTaskRunner()->PostTask(
        FROM_HERE,
        base::Bind(&ForwardingUIService::BindRequestOnGpuThread<Interface,
                                                                Accelerator>,
                   base::Unretained(this), base::Passed(&request)));
  }

  template <typename Interface, typename Accelerator>
  void BindRequestOnGpuThread(mojo::InterfaceRequest<Interface> request) {
    mojo::MakeStrongBinding(base::MakeUnique<Accelerator>(gpu_preferences_),
                            std::move(request));
  }

  ui::Service ui_service_;
  service_manager::BinderRegistry registry_;

  // TODO(penghuang): Correctly initialize gpu::GpuPreferences (like it is
  // initialized in GpuProcessHost::Init()).
  gpu::GpuPreferences gpu_preferences_;

  DISALLOW_COPY_AND_ASSIGN(ForwardingUIService);
};
#endif

std::unique_ptr<service_manager::Service> CreateEmbeddedUIService(
    const scoped_refptr<base::SingleThreadTaskRunner>& task_runner,
    base::WeakPtr<ui::ImageCursorsSet> image_cursors_set_weak_ptr,
    discardable_memory::DiscardableSharedMemoryManager* memory_manager) {
  ui::Service::InProcessConfig config;
  config.resource_runner = task_runner;
  config.image_cursors_set_weak_ptr = image_cursors_set_weak_ptr;
  config.memory_manager = memory_manager;
#if defined(OS_CHROMEOS)
  return std::make_unique<ForwardingUIService>(&config);
#else
  return std::make_unique<ui::Service>(&config);
#endif
}

}  // namespace

service_manager::EmbeddedServiceInfo CreateEmbeddedUIServiceInfo(
    base::WeakPtr<ui::ImageCursorsSet> image_cursors_set_weak_ptr) {
  service_manager::EmbeddedServiceInfo info;
  info.factory = base::Bind(
      &CreateEmbeddedUIService, base::ThreadTaskRunnerHandle::Get(),
      image_cursors_set_weak_ptr, content::GetDiscardableSharedMemoryManager());
  info.use_own_thread = true;
  info.message_loop_type = base::MessageLoop::TYPE_UI;
  info.thread_priority = base::ThreadPriority::DISPLAY;
  return info;
}
