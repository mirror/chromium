// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_TEST_MOCK_RENDER_PROCESS_HOST_IMPL_H_
#define CONTENT_TEST_MOCK_RENDER_PROCESS_HOST_IMPL_H_

#include "content/common/renderer.mojom.h"
#include "content/public/test/mock_render_process_host.h"
#include "mojo/public/cpp/bindings/associated_interface_ptr.h"

namespace content {

// A mock render process host that can test the implementation that's not
// exposed to the content public API.
class MockRenderProcessHostImpl : public MockRenderProcessHost {
 public:
  explicit MockRenderProcessHostImpl(BrowserContext* browser_context);
  ~MockRenderProcessHostImpl() override;

  mojom::Renderer* GetRendererInterface() override;

  // Returns an InterfaceAssociatedRequest to the renderer-side mojom::Renderer
  // implementation. After calling MakeRequestForRendererInterface,
  // GetRendererInterface() returns an interface which is connected to the
  // returned interface request.
  mojom::RendererAssociatedRequest MakeRequestForRendererInterface();

 private:
  mojo::AssociatedInterfacePtr<mojom::Renderer> renderer_interface_;
};

}  // namespace content

#endif  // CONTENT_TEST_MOCK_RENDER_PROCESS_HOST_IMPL_H_
