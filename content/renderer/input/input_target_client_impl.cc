// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/input/input_target_client_impl.h"

#include "base/bind.h"
#include "base/debug/stack_trace.h"
#include "base/logging.h"
#include "content/renderer/render_widget.h"
#include "mojo/public/cpp/bindings/strong_binding.h"

namespace content {

InputTargetClientImpl::InputTargetClientImpl(
    base::WeakPtr<RenderFrameImpl> render_frame)
    : render_frame_(render_frame) {}

InputTargetClientImpl::~InputTargetClientImpl() {}

void InputTargetClientImpl::CreateMojoService(
    base::WeakPtr<RenderFrameImpl> render_frame,
    viz::mojom::InputTargetClientRequest request) {
  DCHECK(render_frame);

  auto* impl = new InputTargetClientImpl(render_frame);
  mojo::MakeStrongBinding(base::WrapUnique(impl), std::move(request));
}

void InputTargetClientImpl::WidgetRoutingIdAt(
    const gfx::Point& point,
    WidgetRoutingIdAtCallback callback) {
  if (!render_frame_)
    return;
  std::move(callback).Run(
      render_frame_->GetRenderWidget()->GetWidgetRoutingIdAtPoint(point));
}

}  // namespace content
