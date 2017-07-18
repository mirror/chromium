// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_LASER_FAST_INK_VIEW_H_
#define ASH_LASER_FAST_INK_VIEW_H_

#include <memory>
#include <vector>

#include "base/containers/flat_map.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "ui/views/view.h"

namespace aura {
class Window;
}

namespace cc {
struct ReturnedResource;
}

namespace gfx {
class GpuMemoryBuffer;
}

namespace views {
class Widget;
}

namespace ash {
class FastPointerLayerTreeFrameSinkHolder;
struct FastPointerResource;

class FastPointerView : public views::View {
 public:
  FastPointerView(aura::Window* root_window);
  ~FastPointerView() override;

 protected:
  views::Widget* GetWidget() const { return widget_.get(); }

  // Unions |rect| with the current damage rect.
  void UpdateDamageRect(const gfx::Rect& rect);

  void RequestRedraw();

  // Draw the pointer image.
  virtual void OnRedraw(gfx::Canvas& canvas,
                        const gfx::Vector2d& widget_offset,
                        const gfx::Rect& update_rect) = 0;

 private:
  friend class FastPointerLayerTreeFrameSinkHolder;

  // Call this to indicate that the previous frame has been processed.
  void DidReceiveCompositorFrameAck();

  // Call this to return resources so they can be reused or freed.
  void ReclaimResources(const std::vector<cc::ReturnedResource>& resources);

  void UpdateBuffer();
  void UpdateSurface();
  void OnDidDrawSurface();

  std::unique_ptr<views::Widget> widget_;
  float scale_factor_ = 1.0f;
  std::unique_ptr<gfx::GpuMemoryBuffer> gpu_memory_buffer_;
  gfx::Rect buffer_damage_rect_;
  bool pending_update_buffer_ = false;
  gfx::Rect surface_damage_rect_;
  bool needs_update_surface_ = false;
  bool pending_draw_surface_ = false;
  std::unique_ptr<FastPointerLayerTreeFrameSinkHolder> frame_sink_holder_;
  int next_resource_id_ = 1;
  base::flat_map<int, std::unique_ptr<FastPointerResource>> resources_;
  std::vector<std::unique_ptr<FastPointerResource>> returned_resources_;
  base::WeakPtrFactory<FastPointerView> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(FastPointerView);
};

}  // namespace ash

#endif  // ASH_LASER_FAST_INK_VIEW_H_
