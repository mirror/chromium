// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/test/mock_render_process_host_impl.h"

#include "base/memory/ptr_util.h"

namespace content {

MockRenderProcessHostImpl::MockRenderProcessHostImpl(
    BrowserContext* browser_context)
    : MockRenderProcessHost(browser_context) {}

MockRenderProcessHostImpl::~MockRenderProcessHostImpl() = default;

mojom::Renderer* MockRenderProcessHostImpl::GetRendererInterface() {
  if (!renderer_interface_)
    mojo::MakeIsolatedRequest(&renderer_interface_);
  return renderer_interface_.get();
}

mojom::RendererAssociatedRequest
MockRenderProcessHostImpl::MakeRequestForRendererInterface() {
  return mojo::MakeIsolatedRequest(&renderer_interface_);
}

}  // namespace content
