// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_MOJO_SERVICES_MOJO_VIDEO_RENDERER_SINK_ADAPTER_H_
#define MEDIA_MOJO_SERVICES_MOJO_VIDEO_RENDERER_SINK_ADAPTER_H_

#include "media/base/video_renderer_sink.h"
#include "media/mojo/interfaces/video_renderer_sink.mojom.h"
#include "media/mojo/services/media_mojo_export.h"
#include "mojo/public/cpp/bindings/associated_binding.h"

namespace media {

class SingleThreadTaskRunner;

class MEDIA_MOJO_EXPORT MojoVideoRendererSinkAdapter
    : public media::VideoRendererSink,
      public mojom::VideoRendererSinkRenderCallback {
 public:
  MojoVideoRendererSinkAdapter(
      const scoped_refptr<base::SingleThreadTaskRunner>& task_runner);

  ~MojoVideoRendererSinkAdapter() final;

  void Initialize(mojom::VideoRendererSinkPtr sink);

  // VideoRendererSink implementation.
  void Start(media::VideoRendererSink::RenderCallback* callback) override;
  void Stop() override;
  void PaintSingleFrame(const scoped_refptr<VideoFrame>& frame,
                        bool repaint_duplicate_frame = false) override;

  // mojom::VideoRendererSinkRenderCallback implementation.
  void Render(base::TimeTicks deadline_min,
              base::TimeTicks deadline_max,
              bool background_rendering,
              media::mojom::VideoRendererSinkRenderCallback::RenderCallback
                  callback) final;
  void OnFrameDropped() final;

 private:
  mojo::AssociatedBinding<mojom::VideoRendererSinkRenderCallback> binding_;
  scoped_refptr<base::SingleThreadTaskRunner> task_runner_;
  bool started_;
  media::VideoRendererSink::RenderCallback* render_callback_;
  mojom::VideoRendererSinkPtr sink_;
};

}  // namespace media

#endif  // MEDIA_MOJO_SERVICES_MOJO_VIDEO_RENDERER_SINK_ADAPTER_H_
