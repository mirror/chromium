// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/test/mock_render_process_host_impl.h"

#include "base/memory/ptr_util.h"

namespace content {

MockRenderProcessHostImpl::MockRenderProcessHostImpl(
    BrowserContext* browser_context)
    : MockRenderProcessHost(browser_context) {}

mojom::RendererAssociatedRequest
MockRenderProcessHostImpl::MakeRequestForRendererInterface() {
  DCHECK(!renderer_interface_);
  renderer_interface_ = base::MakeUnique<mojom::RendererAssociatedPtr>();
  return mojo::MakeIsolatedRequest(renderer_interface_.get());
}

}  // namespace content
