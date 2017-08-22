// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_TEST_MOCK_RENDER_PROCESS_HOST_IMPL_H_
#define CONTENT_TEST_MOCK_RENDER_PROCESS_HOST_IMPL_H_

#include "content/common/renderer.mojom.h"
#include "content/public/test/mock_render_process_host.h"

namespace content {

// A mock render process host that can check content specific implementation.
class MockRenderProcessHostImpl : public MockRenderProcessHost {
 public:
  explicit MockRenderProcessHostImpl(BrowserContext* browser_context);

  // Returns an InterfaceAssociatedReqeust to the renderer-side mojom::Renderer
  // implementation. After calling MakeReqeustForRendererInterface,
  // GetRendererInterface() returns an interface which is connected to the
  // returned interface request.
  mojom::RendererAssociatedRequest MakeRequestForRendererInterface();
};

}  // namespace content

#endif  // CONTENT_TEST_MOCK_RENDER_PROCESS_HOST_IMPL_H_
