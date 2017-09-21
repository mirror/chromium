// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_MOJO_CLIENTS_MOJO_VIDEO_RENDERER_SINK_IMPL_H_
#define MEDIA_MOJO_CLIENTS_MOJO_VIDEO_RENDERER_SINK_IMPL_H_

#include "media/base/video_renderer_sink.h"
#include "media/mojo/interfaces/video_renderer_sink.mojom.h"
#include "mojo/public/cpp/bindings/binding.h"

namespace media {

class MojoVideoRendererSinkImpl
    : public media::VideoRendererSink::RenderCallback,
      public mojom::VideoRendererSink {
 public:
  MojoVideoRendererSinkImpl(
      const scoped_refptr<base::SingleThreadTaskRunner>& task_runner,
      media::VideoRendererSink* sink,
      mojo::InterfaceRequest<mojom::VideoRendererSink> request);
  ~MojoVideoRendererSinkImpl() override;

  // VideoRendererSink::RenderCallback implementation.
  scoped_refptr<VideoFrame> Render(base::TimeTicks deadline_min,
                                   base::TimeTicks deadline_max,
                                   bool background_rendering) override;
  void OnFrameDropped() override;

  // mojom::VideoRendererSink implementation.
  void Start(
      mojom::VideoRendererSinkRenderCallbackAssociatedPtrInfo callback) final;
  void Stop() override;
  void PaintSingleFrame(const scoped_refptr<VideoFrame>& frame,
                        bool repaint_duplicate_frame) final;

  // Sets an error handler that will be called if a connection error occurs on
  // the bound message pipe.
  void set_connection_error_handler(const base::Closure& error_handler) {
    binding_.set_connection_error_handler(error_handler);
  }

 private:
  scoped_refptr<base::SingleThreadTaskRunner> media_task_runner_;
  scoped_refptr<base::SingleThreadTaskRunner> compositor_task_runner_;

  mojo::Binding<mojom::VideoRendererSink> binding_;

  media::VideoRendererSink* sink_;
  bool started_;

  mojom::VideoRendererSinkRenderCallbackAssociatedPtrInfo callback_info_;
  mojom::VideoRendererSinkRenderCallbackAssociatedPtr callback_;
};

}  // namespace media

#endif  // MEDIA_MOJO_CLIENTS_MOJO_VIDEO_RENDERER_SINK_IMPL_H_
