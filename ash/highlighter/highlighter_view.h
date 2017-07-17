// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_HIGHLIGHTER_HIGHLIGHTER_VIEW_H_
#define ASH_HIGHLIGHTER_HIGHLIGHTER_VIEW_H_

#include <memory>
#include <vector>

#include "base/containers/flat_map.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/time/time.h"
#include "ui/gfx/geometry/size_f.h"
#include "ui/views/view.h"

namespace aura {
class Window;
}

namespace base {
class Timer;
}

namespace cc {
struct ReturnedResource;
}

namespace gfx {
class GpuMemoryBuffer;
class PointF;
}  // namespace gfx

namespace views {
class Widget;
}

namespace ash {
class HighlighterLayerTreeFrameSinkHolder;
struct HighlighterResource;

// HighlighterView displays the highlighter palette tool. It draws the
// highlighter which consists of a series of thick lines connecting
// touch points.
//
// The current implementation is a modified copy of LaserPointerView.
// TODO(kaznacheev) Extract a reusable base class from LaserPointerView
// (see crbug.com/743083)
class HighlighterView : public views::View {
 public:
  enum class AnimationMode {
    kFadeout,
    kInflate,
    kDeflate,
  };

  static const SkColor kPenColor;
  static const gfx::SizeF kPenTipSize;

  HighlighterView(aura::Window* root_window);
  ~HighlighterView() override;

  const std::vector<gfx::PointF>& points() const { return points_; }

  void AddNewPoint(const gfx::PointF& new_point);

  // Call this to indicate that the previous frame has been processed.
  void DidReceiveCompositorFrameAck();

  // Call this to return resources so they can be reused or freed.
  void ReclaimResources(const std::vector<cc::ReturnedResource>& resources);

  void Animate(const gfx::PointF& pivot, AnimationMode animation_mode);

 private:
  void OnPointsUpdated();
  void UpdateBuffer();
  void OnBufferUpdated();
  void UpdateSurface();
  void OnDidDrawSurface();

  void FadeOut(const gfx::PointF& pivot, AnimationMode animation_mode);

  std::vector<gfx::PointF> points_;
  std::unique_ptr<base::Timer> animation_timer_;

  std::unique_ptr<views::Widget> widget_;
  float scale_factor_ = 1.0f;
  std::unique_ptr<gfx::GpuMemoryBuffer> gpu_memory_buffer_;
  gfx::Rect buffer_damage_rect_;
  bool pending_update_buffer_ = false;
  gfx::Rect surface_damage_rect_;
  bool needs_update_surface_ = false;
  bool pending_draw_surface_ = false;
  std::unique_ptr<HighlighterLayerTreeFrameSinkHolder> frame_sink_holder_;
  int next_resource_id_ = 1;
  base::flat_map<int, std::unique_ptr<HighlighterResource>> resources_;
  std::vector<std::unique_ptr<HighlighterResource>> returned_resources_;
  base::WeakPtrFactory<HighlighterView> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(HighlighterView);
};

}  // namespace ash

#endif  // ASH_HIGHLIGHTER_HIGHLIGHTER_VIEW_H_
