// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/highlighter/highlighter_view.h"

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <GLES2/gl2extchromium.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <memory>

#include "ash/public/cpp/shell_window_ids.h"
#include "ash/shell.h"
#include "base/containers/adapters.h"
#include "base/memory/ptr_util.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/trace_event/trace_event.h"
#include "cc/output/compositor_frame.h"
#include "cc/output/context_provider.h"
#include "cc/output/layer_tree_frame_sink.h"
#include "cc/output/layer_tree_frame_sink_client.h"
#include "cc/quads/texture_draw_quad.h"
#include "cc/resources/texture_mailbox.h"
#include "cc/resources/transferable_resource.h"
#include "gpu/command_buffer/client/context_support.h"
#include "gpu/command_buffer/client/gles2_interface.h"
#include "gpu/command_buffer/client/gpu_memory_buffer_manager.h"
#include "third_party/skia/include/core/SkColor.h"
#include "third_party/skia/include/core/SkTypes.h"
#include "ui/aura/env.h"
#include "ui/aura/window.h"
#include "ui/base/layout.h"
#include "ui/compositor/scoped_layer_animation_settings.h"
#include "ui/events/base_event_utils.h"
#include "ui/events/event.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/gpu_memory_buffer.h"
#include "ui/views/widget/widget.h"

namespace ash {
namespace {

// Variables for rendering the highlighter. Sizes in DIP.
const SkColor kHighlighterColor = SkColorSetRGB(0x42, 0x85, 0xF4);
const int kHighlighterOpacity = 0xCC;
const float kPenTipWidth = 4;
const float kPenTipHeight = 14;
const int kOutsetForAntialiasing = 1;

const float kStrokeScale = 1.2;

const int kStrokeFadeoutDelayMs = 500;
const int kStrokeFadeoutDurationMs = 500;
const int kStrokeScaleDurationMs = 300;

gfx::RectF GetPenTipRect(const gfx::PointF& p) {
  return gfx::RectF(p.x() - kPenTipWidth / 2, p.y() - kPenTipHeight / 2,
                    kPenTipWidth, kPenTipHeight);
}

gfx::Rect GetSegmentDamageRect(const gfx::RectF& r1, const gfx::RectF& r2) {
  gfx::RectF rect = r1;
  rect.Union(r2);
  rect.Inset(-kOutsetForAntialiasing, -kOutsetForAntialiasing);
  return gfx::ToEnclosingRect(rect);
}

void DrawSegment(gfx::Canvas& canvas,
                 const gfx::RectF& r1,
                 const gfx::RectF& r2,
                 const cc::PaintFlags& flags) {
  if (r1.x() > r2.x()) {
    DrawSegment(canvas, r2, r1, flags);
    return;
  }

  SkPath path;
  path.moveTo(r1.x(), r1.y());
  if (r1.y() < r2.y())
    path.lineTo(r1.right(), r1.y());
  else
    path.lineTo(r2.x(), r2.y());
  path.lineTo(r2.right(), r2.y());
  path.lineTo(r2.right(), r2.bottom());
  if (r1.y() < r2.y())
    path.lineTo(r2.x(), r2.bottom());
  else
    path.lineTo(r1.right(), r1.bottom());
  path.lineTo(r1.x(), r1.bottom());
  path.lineTo(r1.x(), r1.y());
  canvas.DrawPath(path, flags);
}

}  // namespace

class HighlighterLayerTreeFrameSinkHolder
    : public cc::LayerTreeFrameSinkClient {
 public:
  HighlighterLayerTreeFrameSinkHolder(
      HighlighterView* view,
      std::unique_ptr<cc::LayerTreeFrameSink> frame_sink)
      : view_(view), frame_sink_(std::move(frame_sink)) {
    frame_sink_->BindToClient(this);
  }
  ~HighlighterLayerTreeFrameSinkHolder() override {
    frame_sink_->DetachFromClient();
  }

  cc::LayerTreeFrameSink* frame_sink() { return frame_sink_.get(); }

  // Called before highlighter view is destroyed.
  void OnHighlighterViewDestroying() { view_ = nullptr; }

  // Overridden from cc::LayerTreeFrameSinkClient:
  void SetBeginFrameSource(cc::BeginFrameSource* source) override {}
  void ReclaimResources(const cc::ReturnedResourceArray& resources) override {
    if (view_)
      view_->ReclaimResources(resources);
  }
  void SetTreeActivationCallback(const base::Closure& callback) override {}
  void DidReceiveCompositorFrameAck() override {
    if (view_)
      view_->DidReceiveCompositorFrameAck();
  }
  void DidLoseLayerTreeFrameSink() override {}
  void OnDraw(const gfx::Transform& transform,
              const gfx::Rect& viewport,
              bool resourceless_software_draw) override {}
  void SetMemoryPolicy(const cc::ManagedMemoryPolicy& policy) override {}
  void SetExternalTilePriorityConstraints(
      const gfx::Rect& viewport_rect,
      const gfx::Transform& transform) override {}

 private:
  HighlighterView* view_;
  std::unique_ptr<cc::LayerTreeFrameSink> frame_sink_;

  DISALLOW_COPY_AND_ASSIGN(HighlighterLayerTreeFrameSinkHolder);
};

// This struct contains the resources associated with a highlighter frame.
struct HighlighterResource {
  HighlighterResource() {}
  ~HighlighterResource() {
    if (context_provider) {
      gpu::gles2::GLES2Interface* gles2 = context_provider->ContextGL();
      if (texture)
        gles2->DeleteTextures(1, &texture);
      if (image)
        gles2->DestroyImageCHROMIUM(image);
    }
  }
  scoped_refptr<cc::ContextProvider> context_provider;
  uint32_t texture = 0;
  uint32_t image = 0;
  gpu::Mailbox mailbox;
};

// HighlighterView
HighlighterView::HighlighterView(aura::Window* root_window)
    : weak_ptr_factory_(this) {
  widget_.reset(new views::Widget);
  views::Widget::InitParams params;
  params.type = views::Widget::InitParams::TYPE_WINDOW_FRAMELESS;
  params.name = "HighlighterOverlay";
  params.accept_events = false;
  params.activatable = views::Widget::InitParams::ACTIVATABLE_NO;
  params.ownership = views::Widget::InitParams::WIDGET_OWNS_NATIVE_WIDGET;
  params.opacity = views::Widget::InitParams::TRANSLUCENT_WINDOW;
  params.parent =
      Shell::GetContainer(root_window, kShellWindowId_OverlayContainer);
  params.layer_type = ui::LAYER_SOLID_COLOR;

  widget_->Init(params);
  widget_->Show();
  widget_->SetContentsView(this);
  widget_->SetBounds(root_window->GetBoundsInScreen());
  set_owned_by_client();

  scale_factor_ = ui::GetScaleFactorForNativeView(widget_->GetNativeView());

  frame_sink_holder_ = base::MakeUnique<HighlighterLayerTreeFrameSinkHolder>(
      this, widget_->GetNativeView()->CreateLayerTreeFrameSink());
}

HighlighterView::~HighlighterView() {
  frame_sink_holder_->OnHighlighterViewDestroying();
}

SkColor HighlighterView::GetPenColor() const {
  return SkColorSetA(kHighlighterColor, kHighlighterOpacity);
}

gfx::Size HighlighterView::GetPenTipSize() const {
  return gfx::Size(kPenTipWidth, kPenTipHeight);
}

gfx::Rect HighlighterView::GetBoundingBox() const {
  if (points_.empty())
    return gfx::Rect();

  return gfx::ToEnclosingRect(gfx::BoundingRect(min_point_, max_point_));
}

void HighlighterView::AddNewPoint(const gfx::PointF& point) {
  TRACE_EVENT1("ui", "HighlighterView::AddNewPoint", "point", point.ToString());

  if (points_.empty()) {
    min_point_ = point;
    max_point_ = point;
  } else {
    min_point_.SetToMin(point);
    max_point_.SetToMax(point);
    buffer_damage_rect_.Union(GetSegmentDamageRect(
        GetPenTipRect(points_.back()), GetPenTipRect(point)));
  }

  points_.push_back(point);

  OnPointsUpdated();
}

void HighlighterView::DidReceiveCompositorFrameAck() {
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::Bind(&HighlighterView::OnDidDrawSurface,
                            weak_ptr_factory_.GetWeakPtr()));
}

void HighlighterView::ReclaimResources(
    const cc::ReturnedResourceArray& resources) {
  DCHECK_EQ(resources.size(), 1u);

  auto it = resources_.find(resources.front().id);
  DCHECK(it != resources_.end());
  std::unique_ptr<HighlighterResource> resource = std::move(it->second);
  resources_.erase(it);

  gpu::gles2::GLES2Interface* gles2 = resource->context_provider->ContextGL();
  if (resources.front().sync_token.HasData())
    gles2->WaitSyncTokenCHROMIUM(resources.front().sync_token.GetConstData());

  if (!resources.front().lost)
    returned_resources_.push_back(std::move(resource));
}

void HighlighterView::AnimateInPlace() {
  Animate(1);
}

void HighlighterView::AnimateInflate() {
  Animate(kStrokeScale);
}

void HighlighterView::AnimateDeflate() {
  Animate(1 / kStrokeScale);
}

void HighlighterView::Animate(float scale) {
  animation_timer_.reset(new base::Timer(
      FROM_HERE, base::TimeDelta::FromMilliseconds(kStrokeFadeoutDelayMs),
      base::Bind(&HighlighterView::FadeOut, base::Unretained(this), scale),
      false));
  animation_timer_->Reset();
}

void HighlighterView::FadeOut(float scale) {
  ui::Layer* layer = widget_->GetLayer();

  {
    ui::ScopedLayerAnimationSettings settings(layer->GetAnimator());
    settings.SetTransitionDuration(
        base::TimeDelta::FromMilliseconds(kStrokeFadeoutDurationMs));
    settings.SetTweenType(gfx::Tween::LINEAR_OUT_SLOW_IN);

    layer->SetOpacity(0);
  }

  {
    ui::ScopedLayerAnimationSettings settings(layer->GetAnimator());
    settings.SetTransitionDuration(
        base::TimeDelta::FromMilliseconds(kStrokeScaleDurationMs));
    settings.SetTweenType(gfx::Tween::LINEAR_OUT_SLOW_IN);

    gfx::Transform transform;
    const gfx::Point pivot = GetBoundingBox().CenterPoint();
    transform.Translate(pivot.x() * (1 - scale), pivot.y() * (1 - scale));
    transform.Scale(scale, scale);

    layer->SetTransform(transform);
  }
}

void HighlighterView::OnPointsUpdated() {
  if (pending_update_buffer_)
    return;

  pending_update_buffer_ = true;
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::Bind(&HighlighterView::UpdateBuffer,
                            weak_ptr_factory_.GetWeakPtr()));
}

void HighlighterView::UpdateBuffer() {
  TRACE_EVENT1("ui", "HighlighterView::UpdateBuffer", "damage",
               buffer_damage_rect_.ToString());

  DCHECK(pending_update_buffer_);
  pending_update_buffer_ = false;

  gfx::Rect screen_bounds = widget_->GetNativeView()->GetBoundsInScreen();
  gfx::Rect update_rect = buffer_damage_rect_;
  buffer_damage_rect_ = gfx::Rect();

  // Create and map a single GPU memory buffer. The highlighter will be
  // written into this buffer without any buffering. The result is that we
  // might be modifying the buffer while it's being displayed. This provides
  // minimal latency but potential tearing. Note that we have to draw into
  // a temporary surface and copy it into GPU memory buffer to avoid flicker.
  if (!gpu_memory_buffer_) {
    TRACE_EVENT0("ui", "HighlighterView::UpdateBuffer::Create");

    gpu_memory_buffer_ =
        aura::Env::GetInstance()
            ->context_factory()
            ->GetGpuMemoryBufferManager()
            ->CreateGpuMemoryBuffer(
                gfx::ScaleToCeiledSize(screen_bounds.size(), scale_factor_),
                SK_B32_SHIFT ? gfx::BufferFormat::RGBA_8888
                             : gfx::BufferFormat::BGRA_8888,
                gfx::BufferUsage::SCANOUT_CPU_READ_WRITE,
                gpu::kNullSurfaceHandle);
    if (!gpu_memory_buffer_) {
      LOG(ERROR) << "Failed to allocate GPU memory buffer";
      return;
    }

    // Make sure the first update rectangle covers the whole buffer.
    update_rect = gfx::Rect(screen_bounds.size());
  }

  // Constrain update rectangle to buffer size and early out if empty.
  update_rect.Intersect(gfx::Rect(screen_bounds.size()));
  if (update_rect.IsEmpty())
    return;

  // Map buffer for writing.
  if (!gpu_memory_buffer_->Map()) {
    LOG(ERROR) << "Failed to map GPU memory buffer";
    return;
  }

  // Create a temporary canvas for update rectangle.
  gfx::Canvas canvas(update_rect.size(), scale_factor_, false);

  cc::PaintFlags flags;
  flags.setStyle(cc::PaintFlags::kFill_Style);
  flags.setAntiAlias(true);
  flags.setColor(GetPenColor());
  flags.setBlendMode(SkBlendMode::kSrc);

  // Compute the offset of the current widget.
  const gfx::Vector2d widget_offset(
      widget_->GetNativeView()->GetBoundsInRootWindow().origin().x(),
      widget_->GetNativeView()->GetBoundsInRootWindow().origin().y());

  const gfx::Vector2d canvas_offset =
      widget_offset + update_rect.OffsetFromOrigin();

  if (points_.size() < 2)
    return;

  TRACE_EVENT1("ui", "HighlighterView::UpdateBuffer::Paint", "update_rect",
               update_rect.ToString());

  for (size_t i = 1; i < points_.size(); ++i) {
    const gfx::RectF tip1 = GetPenTipRect(points_[i - 1]);
    const gfx::RectF tip2 = GetPenTipRect(points_[i]);
    // Only draw the segment if it touches the update rect.
    if (update_rect.Intersects(GetSegmentDamageRect(tip1, tip2)))
      DrawSegment(canvas, tip1 - canvas_offset, tip2 - canvas_offset, flags);
  }

  // Convert update rectangle to pixel coordinates.
  gfx::Rect pixel_rect = gfx::ScaleToEnclosingRect(update_rect, scale_factor_);

  // Copy result to GPU memory buffer. This is effectiely a memcpy and unlike
  // drawing to the buffer directly this ensures that the buffer is never in a
  // state that would result in flicker.
  {
    TRACE_EVENT1("ui", "HighlighterView::UpdateBuffer::Copy", "pixel_rect",
                 pixel_rect.ToString());

    uint8_t* data = static_cast<uint8_t*>(gpu_memory_buffer_->memory(0));
    int stride = gpu_memory_buffer_->stride(0);
    canvas.GetBitmap().readPixels(
        SkImageInfo::MakeN32Premul(pixel_rect.width(), pixel_rect.height()),
        data + pixel_rect.y() * stride + pixel_rect.x() * 4, stride, 0, 0);
  }

  // Unmap to flush writes to buffer.
  gpu_memory_buffer_->Unmap();

  // Update surface damage rectangle.
  surface_damage_rect_.Union(update_rect);

  needs_update_surface_ = true;

  // Early out if waiting for last surface update to be drawn.
  if (pending_draw_surface_)
    return;

  UpdateSurface();
}

void HighlighterView::UpdateSurface() {
  TRACE_EVENT1("ui", "HighlighterView::UpdateSurface", "damage",
               surface_damage_rect_.ToString());

  DCHECK(needs_update_surface_);
  needs_update_surface_ = false;

  std::unique_ptr<HighlighterResource> resource;
  // Reuse returned resource if available.
  if (!returned_resources_.empty()) {
    resource = std::move(returned_resources_.back());
    returned_resources_.pop_back();
  }

  // Create new resource if needed.
  if (!resource)
    resource = base::MakeUnique<HighlighterResource>();

  // Acquire context provider for resource if needed.
  // Note: We make no attempts to recover if the context provider is later
  // lost. It is expected that this class is short-lived and requiring a
  // new instance to be created in lost context situations is acceptable and
  // keeps the code simple.
  if (!resource->context_provider) {
    resource->context_provider = aura::Env::GetInstance()
                                     ->context_factory()
                                     ->SharedMainThreadContextProvider();
    if (!resource->context_provider) {
      LOG(ERROR) << "Failed to acquire a context provider";
      return;
    }
  }

  gpu::gles2::GLES2Interface* gles2 = resource->context_provider->ContextGL();

  if (resource->texture) {
    gles2->ActiveTexture(GL_TEXTURE0);
    gles2->BindTexture(GL_TEXTURE_2D, resource->texture);
  } else {
    gles2->GenTextures(1, &resource->texture);
    gles2->ActiveTexture(GL_TEXTURE0);
    gles2->BindTexture(GL_TEXTURE_2D, resource->texture);
    gles2->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    gles2->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    gles2->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    gles2->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    gles2->GenMailboxCHROMIUM(resource->mailbox.name);
    gles2->ProduceTextureCHROMIUM(GL_TEXTURE_2D, resource->mailbox.name);
  }

  gfx::Size buffer_size = gpu_memory_buffer_->GetSize();

  if (resource->image) {
    gles2->ReleaseTexImage2DCHROMIUM(GL_TEXTURE_2D, resource->image);
  } else {
    resource->image = gles2->CreateImageCHROMIUM(
        gpu_memory_buffer_->AsClientBuffer(), buffer_size.width(),
        buffer_size.height(), SK_B32_SHIFT ? GL_RGBA : GL_BGRA_EXT);
    if (!resource->image) {
      LOG(ERROR) << "Failed to create image";
      return;
    }
  }
  gles2->BindTexImage2DCHROMIUM(GL_TEXTURE_2D, resource->image);

  gpu::SyncToken sync_token;
  uint64_t fence_sync = gles2->InsertFenceSyncCHROMIUM();
  gles2->OrderingBarrierCHROMIUM();
  gles2->GenUnverifiedSyncTokenCHROMIUM(fence_sync, sync_token.GetData());

  cc::TransferableResource transferable_resource;
  transferable_resource.id = next_resource_id_++;
  transferable_resource.format = cc::RGBA_8888;
  transferable_resource.filter = GL_LINEAR;
  transferable_resource.size = buffer_size;
  transferable_resource.mailbox_holder =
      gpu::MailboxHolder(resource->mailbox, sync_token, GL_TEXTURE_2D);
  transferable_resource.is_overlay_candidate = true;

  gfx::Rect quad_rect(widget_->GetNativeView()->GetBoundsInScreen().size());

  const int kRenderPassId = 1;
  std::unique_ptr<cc::RenderPass> render_pass = cc::RenderPass::Create();
  render_pass->SetNew(kRenderPassId, quad_rect, surface_damage_rect_,
                      gfx::Transform());
  surface_damage_rect_ = gfx::Rect();

  cc::SharedQuadState* quad_state =
      render_pass->CreateAndAppendSharedQuadState();
  quad_state->quad_layer_rect = quad_rect;
  quad_state->visible_quad_layer_rect = quad_rect;
  quad_state->opacity = 1.0f;

  cc::CompositorFrame frame;
  // TODO(eseckler): HighlighterView should use BeginFrames and set the ack
  // accordingly.
  frame.metadata.begin_frame_ack =
      cc::BeginFrameAck::CreateManualAckWithDamage();
  frame.metadata.device_scale_factor =
      widget_->GetLayer()->device_scale_factor();
  cc::TextureDrawQuad* texture_quad =
      render_pass->CreateAndAppendDrawQuad<cc::TextureDrawQuad>();
  float vertex_opacity[4] = {1.0, 1.0, 1.0, 1.0};
  gfx::PointF uv_top_left(0.f, 0.f);
  gfx::PointF uv_bottom_right(1.f, 1.f);
  texture_quad->SetNew(quad_state, quad_rect, gfx::Rect(), quad_rect,
                       transferable_resource.id, true, uv_top_left,
                       uv_bottom_right, SK_ColorTRANSPARENT, vertex_opacity,
                       false, false, false);
  texture_quad->set_resource_size_in_pixels(transferable_resource.size);
  frame.resource_list.push_back(transferable_resource);
  frame.render_pass_list.push_back(std::move(render_pass));

  frame_sink_holder_->frame_sink()->SubmitCompositorFrame(std::move(frame));

  resources_[transferable_resource.id] = std::move(resource);

  DCHECK(!pending_draw_surface_);
  pending_draw_surface_ = true;
}

void HighlighterView::OnDidDrawSurface() {
  pending_draw_surface_ = false;
  if (needs_update_surface_)
    UpdateSurface();
}

}  // namespace ash
