// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_INPUT_TARGET_FRAME_FOR_INPUT_IMPL_H_
#define CONTENT_RENDERER_INPUT_TARGET_FRAME_FOR_INPUT_IMPL_H_

#include "base/memory/ref_counted.h"
#include "content/renderer/render_frame_impl.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "services/viz/public/interfaces/hit_test/target_frame_for_input_delegate.mojom.h"

namespace content {
class MainThreadEventQueue;

// This class provides an implementation of FrameInputHitTest mojo interface.
class TargetFrameForInputImpl : public viz::mojom::TargetFrameForInputDelegate {
 public:
  static void CreateMojoService(
      base::WeakPtr<RenderFrameImpl> render_frame,
      viz::mojom::TargetFrameForInputDelegateRequest request);

  void HitTestFrameAt(const gfx::Point& point,
                      HitTestFrameAtCallback callback) override;

 private:
  ~TargetFrameForInputImpl() override;

  TargetFrameForInputImpl(
      base::WeakPtr<RenderFrameImpl> render_frame,
      viz::mojom::TargetFrameForInputDelegateRequest request);

  void RunOnMainThread(base::OnceClosure closure);
  void HitTestOnMainThread(const gfx::Point&, HitTestFrameAtCallback);

  void BindNow(viz::mojom::TargetFrameForInputDelegateRequest request);
  void Release();

  mojo::Binding<viz::mojom::TargetFrameForInputDelegate> binding_;

  // |render_frame_| should only be accessed on the main thread. Use
  // GetRenderFrame so that it will DCHECK this for you.
  base::WeakPtr<RenderFrameImpl> render_frame_;

  scoped_refptr<MainThreadEventQueue> input_event_queue_;
  scoped_refptr<base::SingleThreadTaskRunner> main_thread_task_runner_;

  base::WeakPtr<TargetFrameForInputImpl> weak_this_;
  base::WeakPtrFactory<TargetFrameForInputImpl> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(TargetFrameForInputImpl);
};

}  // namespace content

#endif  // CONTENT_RENDERER_INPUT_TARGET_FRAME_FOR_INPUT_IMPL_H_
