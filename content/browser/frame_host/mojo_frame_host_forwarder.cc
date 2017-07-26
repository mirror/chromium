// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/frame_host/mojo_frame_host_forwarder.h"

#include <utility>

#include "base/logging.h"

namespace content {

MojoFrameHostForwarder::MojoFrameHostForwarder(mojom::FrameHost* receiver)
    : receiver_(receiver) {}

MojoFrameHostForwarder::~MojoFrameHostForwarder() {}

bool MojoFrameHostForwarder::CreateNewWindow(
    mojom::CreateNewWindowParamsPtr params,
    mojom::CreateNewWindowReplyPtr* reply) {
  DCHECK(receiver_);
  return receiver_->CreateNewWindow(std::move(params), reply);
}

void MojoFrameHostForwarder::CreateNewWindow(
    mojom::CreateNewWindowParamsPtr params,
    CreateNewWindowCallback callback) {
  DCHECK(receiver_);
  receiver_->CreateNewWindow(std::move(params), std::move(callback));
}

void MojoFrameHostForwarder::BindInterfaceProviderForInitialEmptyDocument(
    ::service_manager::mojom::InterfaceProviderRequest request) {
  DCHECK(receiver_);
  receiver_->BindInterfaceProviderForInitialEmptyDocument(std::move(request));
}

void MojoFrameHostForwarder::DidCommitProvisionalLoad(
    mojom::DidCommitProvisionalLoadParamsPtr params) {
  DCHECK(receiver_);
  receiver_->DidCommitProvisionalLoad(std::move(params));
}

}  // namespace content
