// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_FRAME_HOST_RENDER_WIDGET_HOST_VIEW_MUS_GUEST_H_
#define CONTENT_BROWSER_FRAME_HOST_RENDER_WIDGET_HOST_VIEW_MUS_GUEST_H_

#include "content/browser/renderer_host/render_widget_host_view_base.h"

namespace content {

// RenderWidgetHostView implementation used by RenderWidgetHostViewGuest when
// in mus.
class CONTENT_EXPORT RenderWidgetHostViewMusGuest
    : public RenderWidgetHostViewBase {
 public:
  RenderWidgetHostViewMusGuest();
  ~RenderWidgetHostViewMusGuest() override;

  // RenderWidgetHostView:
  void InitAsChild(gfx::NativeView parent_view) override;
  void SetSize(const gfx::Size& size) override;
  void SetBounds(const gfx::Rect& rect) override;
  gfx::Vector2dF GetLastScrollOffset() const override;
  gfx::NativeView GetNativeView() const override;
  gfx::NativeViewAccessible GetNativeViewAccessible() override;
  void Focus() override;
  bool HasFocus() const override;
  void Show() override;
  void Hide() override;
  bool IsShowing() override;
  gfx::Rect GetViewBounds() const override;
  void SetBackgroundColor(SkColor color) override;
  SkColor background_color() const override;
  void SetNeedsBeginFrames(bool needs_begin_frames) override;
  void SubmitCompositorFrame(const viz::LocalSurfaceId& local_surface_id,
                             viz::CompositorFrame frame) override;
  void ClearCompositorFrame() override;
  void InitAsPopup(RenderWidgetHostView* parent_host_view,
                   const gfx::Rect& bounds) override;
  void InitAsFullscreen(RenderWidgetHostView* reference_host_view) override;
  void UpdateCursor(const WebCursor& cursor) override;
  void SetTooltipText(const base::string16& tooltip_text) override;
  bool HasAcceleratedSurface(const gfx::Size& desired_size) override;
  gfx::Rect GetBoundsInRootWindow() override;

  void RenderProcessGone(base::TerminationStatus status,
                         int error_code) override;
  void Destroy() override;
  base::string16 GetSelectedText() override;
  void SetIsLoading(bool is_loading) override;
  void SelectionChanged(const base::string16& text,
                        size_t offset,
                        const gfx::Range& range) override;
  bool LockMouse() override;
  void UnlockMouse() override;
  void DidCreateNewRendererCompositorFrameSink(
      viz::mojom::CompositorFrameSinkClient* renderer_compositor_frame_sink)
      override;

 private:
  DISALLOW_COPY_AND_ASSIGN(RenderWidgetHostViewMusGuest);
};

}  // namespace content

#endif  // CONTENT_BROWSER_FRAME_HOST_RENDER_WIDGET_HOST_VIEW_MUS_GUEST_H_
