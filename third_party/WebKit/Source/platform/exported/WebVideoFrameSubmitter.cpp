// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "public/platform/WebVideoFrameSubmitter.h"

#include "public/platform/InterfaceProvider.h"
#include "public/platform/Platform.h"
#include "public/platform/modules/offscreencanvas/offscreen_canvas_surface.mojom-blink.h"
#include "services/viz/public/interfaces/compositing/compositor_frame_sink.mojom-blink.h"
#include "third_party/WebKit/Source/platform/graphics/VideoFrameSubmitter.h"
#include "third_party/WebKit/Source/platform/wtf/Functional.h"

namespace cc {
class VideoFrameProvider;
}  // namespace cc

namespace blink {

std::unique_ptr<WebVideoFrameSubmitter> WebVideoFrameSubmitter::Create(
    cc::VideoFrameProvider* provider,
    const viz::FrameSinkId& id) {
  DCHECK(id.is_valid());

  VideoFrameSubmitter* submitter = new VideoFrameSubmitter(provider);
  mojo::Binding<viz::mojom::blink::CompositorFrameSinkClient>* binding =
      submitter->Binding();

  // Class to be renamed.
  mojom::blink::OffscreenCanvasProviderPtr canvas_provider;
  Platform::Current()->GetInterfaceProvider()->GetInterface(
      mojo::MakeRequest(&canvas_provider));

  viz::mojom::blink::CompositorFrameSinkClientPtr client;
  viz::mojom::blink::CompositorFrameSinkPtr sink;
  binding->Bind(mojo::MakeRequest(&client));
  canvas_provider->CreateCompositorFrameSink(id, std::move(client),
                                             mojo::MakeRequest(&sink));
  submitter->SetSink(&sink);

  return base::WrapUnique<VideoFrameSubmitter>(submitter);
}

}  // namespace blink
