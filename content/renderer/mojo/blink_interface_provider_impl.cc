// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/mojo/blink_interface_provider_impl.h"

#include <utility>

#include "base/bind.h"
#include "base/single_thread_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "content/public/common/service_names.mojom.h"
#include "content/public/renderer/render_frame.h"
#include "services/service_manager/public/cpp/connector.h"
#include "services/service_manager/public/cpp/interface_provider.h"

namespace content {

BlinkInterfaceProviderImpl::BlinkInterfaceProviderImpl(
    base::WeakPtr<service_manager::Connector> connector)
    : connector_(connector),
      main_thread_task_runner_(base::ThreadTaskRunnerHandle::Get()),
      weak_ptr_factory_(this) {
  weak_ptr_ = weak_ptr_factory_.GetWeakPtr();
}

BlinkInterfaceProviderImpl::BlinkInterfaceProviderImpl(RenderFrame* frame)
    : frame_(frame),
      main_thread_task_runner_(base::ThreadTaskRunnerHandle::Get()),
      weak_ptr_factory_(this) {
  weak_ptr_ = weak_ptr_factory_.GetWeakPtr();
}

BlinkInterfaceProviderImpl::~BlinkInterfaceProviderImpl() = default;

void BlinkInterfaceProviderImpl::GetInterface(
    const char* name,
    mojo::ScopedMessagePipeHandle handle) {
  GetInterfaceInternal(name, std::move(handle));
}

void BlinkInterfaceProviderImpl::GetInterfaceInternal(
    const std::string& name,
    mojo::ScopedMessagePipeHandle handle) {
  if (!main_thread_task_runner_->BelongsToCurrentThread()) {
    // TODO(rockot): We should consider introducing a Clone() API for
    // InterfaceProvider so that we can maintain a TLS cache of
    // InterfaceProviders to the same browser-side binding. This would avoid
    // extra thread-hopping.
    main_thread_task_runner_->PostTask(
        FROM_HERE,
        base::BindOnce(&BlinkInterfaceProviderImpl::GetInterfaceInternal,
                       weak_ptr_, name, std::move(handle)));
    return;
  }

  if (connector_) {
    connector_->BindInterface(
        service_manager::Identity(mojom::kBrowserServiceName,
                                  service_manager::mojom::kInheritUserID),
        name, std::move(handle));
  } else {
    frame_->GetRemoteInterfaces()->GetInterface(name, std::move(handle));
  }
}

}  // namespace content
