// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_INPUT_INPUT_TARGET_CLIENT_IMPL_H_
#define CONTENT_RENDERER_INPUT_INPUT_TARGET_CLIENT_IMPL_H_

#include "base/memory/ref_counted.h"
#include "content/renderer/render_frame_impl.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "services/viz/public/interfaces/hit_test/input_target_client.mojom.h"

namespace content {
class MainThreadEventQueue;

// This class provides an implementation of InputTargetClient mojo interface.
class InputTargetClientImpl : public viz::mojom::InputTargetClient {
 public:
  static void CreateMojoService(base::WeakPtr<RenderFrameImpl> render_frame,
                                viz::mojom::InputTargetClientRequest request);

  void WidgetRoutingIdAt(const gfx::Point& point,
                         WidgetRoutingIdAtCallback callback) override;

 private:
  ~InputTargetClientImpl() override;

  InputTargetClientImpl(base::WeakPtr<RenderFrameImpl> render_frame,
                        viz::mojom::InputTargetClientRequest request);

  void RunOnMainThread(base::OnceClosure closure);
  void HitTestOnMainThread(const gfx::Point&, WidgetRoutingIdAtCallback);

  void BindNow(viz::mojom::InputTargetClientRequest request);
  void Release();

  mojo::Binding<viz::mojom::InputTargetClient> binding_;

  // |render_frame_| should only be accessed on the main thread. Use
  // GetRenderFrame so that it will DCHECK this for you.
  base::WeakPtr<RenderFrameImpl> render_frame_;

  scoped_refptr<MainThreadEventQueue> input_event_queue_;
  scoped_refptr<base::SingleThreadTaskRunner> main_thread_task_runner_;

  base::WeakPtr<InputTargetClientImpl> weak_this_;
  base::WeakPtrFactory<InputTargetClientImpl> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(InputTargetClientImpl);
};

}  // namespace content

#endif  // CONTENT_RENDERER_INPUT_INPUT_TARGET_CLIENT_IMPL_H_
