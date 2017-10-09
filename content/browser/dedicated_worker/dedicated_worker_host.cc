// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string>

#include "content/browser/dedicated_worker/dedicated_worker_host.h"
#include "content/browser/worker_binder_registry.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "mojo/public/cpp/bindings/strong_associated_binding.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "mojo/public/cpp/system/message_pipe.h"
#include "services/service_manager/public/cpp/binder_registry.h"
#include "services/service_manager/public/interfaces/interface_provider.mojom.h"
#include "url/origin.h"

namespace content {
namespace {

class DedicatedWorkerHost : public service_manager::mojom::InterfaceProvider {
 public:
  DedicatedWorkerHost(int process_id, const url::Origin& origin)
      : process_id_(process_id), origin_(origin) {}

  void GetInterface(const std::string& interface_name,
                    mojo::ScopedMessagePipeHandle interface_pipe) override {
    RenderProcessHost* process = RenderProcessHost::FromID(process_id_);
    if (!process)
      return;

    GetWorkerBinderRegistry().BindInterface(
        interface_name, std::move(interface_pipe), process, origin_);
  }

 private:
  const int process_id_;
  const url::Origin origin_;

  DISALLOW_COPY_AND_ASSIGN(DedicatedWorkerHost);
};

class DedicatedWorkerFactoryImpl : public blink::mojom::DedicatedWorkerFactory {
 public:
  DedicatedWorkerFactoryImpl(int process_id, const url::Origin& origin)
      : process_id_(process_id), origin_(origin) {}

  void CreateDedicatedWorker(
      bool unique_origin,
      service_manager::mojom::InterfaceProviderRequest request) override {
    mojo::MakeStrongBinding(
        base::MakeUnique<DedicatedWorkerHost>(
            process_id_, unique_origin ? url::Origin() : origin_),
        std::move(request));
  }

 private:
  const int process_id_;
  const url::Origin origin_;

  DISALLOW_COPY_AND_ASSIGN(DedicatedWorkerFactoryImpl);
};

}  // namespace

void CreateDedicatedWorkerHostFactory(
    int process_id,
    RenderFrameHost* frame,
    blink::mojom::DedicatedWorkerFactoryAssociatedRequest request) {
  mojo::MakeStrongAssociatedBinding(
      base::MakeUnique<DedicatedWorkerFactoryImpl>(
          process_id, frame->GetLastCommittedOrigin()),
      std::move(request));
}

}  // namespace content
