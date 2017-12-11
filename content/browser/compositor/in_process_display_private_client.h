// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_COMPOSITOR_IN_PROCESS_DISPLAY_PRIVATE_CLIENT_H_
#define CONTENT_BROWSER_COMPOSITOR_IN_PROCESS_DISPLAY_PRIVATE_CLIENT_H_

#include "build/build_config.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "services/viz/privileged/interfaces/compositing/display_private.mojom.h"
#include "ui/gfx/native_widget_types.h"

namespace content {

// A DisplayPrivateClient that can be used to display received
// gfx::CALayerParams in a CALayer tree in this process.
class InProcessDisplayPrivateClient : public viz::mojom::DisplayPrivateClient {
 public:
  InProcessDisplayPrivateClient(gfx::AcceleratedWidget widget);
  ~InProcessDisplayPrivateClient() override;

  viz::mojom::DisplayPrivateClientPtr GetBoundPtr();

 private:
  // viz::mojom::DisplayPrivateClient implementation:
  void OnDisplayReceivedCALayerParams(
      const gfx::CALayerParams& ca_layer_params) override;

  mojo::Binding<viz::mojom::DisplayPrivateClient> binding_;
#if defined(OS_MACOSX)
  gfx::AcceleratedWidget widget_;
#endif
};

}  // namespace content

#endif  // CONTENT_BROWSER_COMPOSITOR_IN_PROCESS_DISPLAY_PRIVATE_CLIENT_H_
