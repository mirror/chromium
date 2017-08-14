// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_RENDERER_HOST_FRAME_CONNECTOR_DELEGATE_H_
#define CONTENT_BROWSER_RENDERER_HOST_FRAME_CONNECTOR_DELEGATE_H_

#include "content/browser/renderer_host/event_with_latency_info.h"
#include "content/common/content_export.h"
#include "content/common/input/input_event_ack_state.h"
#include "ui/gfx/geometry/rect.h"

namespace blink {
class WebGestureEvent;
}

namespace viz {
class SurfaceId;
class SurfaceInfo;
struct SurfaceSequence;
}  // namespace viz

namespace content {
class RenderWidgetHostViewBase;
class RenderWidgetHostViewChildFrame;
class WebCursor;

class CONTENT_EXPORT FrameConnectorDelegate {
 public:
  virtual void set_view(RenderWidgetHostViewChildFrame* view) {}

  // Returns the parent RenderWidgetHostView or nullptr it it doesn't have one.
  virtual RenderWidgetHostViewBase* GetParentRenderWidgetHostView();

  // Returns the view for the top-level frame under the same WebContents.
  virtual RenderWidgetHostViewBase* GetRootRenderWidgetHostView();

  virtual void RenderProcessGone() {}

  virtual void SetChildFrameSurface(const viz::SurfaceInfo& surface_info,
                                    const viz::SurfaceSequence& sequence) {}

  virtual gfx::Rect ChildFrameRect();
  virtual void UpdateCursor(const WebCursor& cursor) {}
  virtual gfx::Point TransformPointToRootCoordSpace(
      const gfx::Point& point,
      const viz::SurfaceId& surface_id);
  // TransformPointToLocalCoordSpace() can only transform points between
  // surfaces where one is embedded (not necessarily directly) within the
  // other, and will return false if this is not the case. For points that can
  // be in sibling surfaces, they must first be converted to the root
  // surface's coordinate space.
  virtual bool TransformPointToLocalCoordSpace(
      const gfx::Point& point,
      const viz::SurfaceId& original_surface,
      const viz::SurfaceId& local_surface_id,
      gfx::Point* transformed_point);
  // Returns false if |target_view| and |view_| do not have the same root
  // RenderWidgetHostView.
  virtual bool TransformPointToCoordSpaceForView(
      const gfx::Point& point,
      RenderWidgetHostViewBase* target_view,
      const viz::SurfaceId& local_surface_id,
      gfx::Point* transformed_point);

  // Pass acked touch events to the root view for gesture processing.
  virtual void ForwardProcessAckedTouchEvent(
      const TouchEventWithLatencyInfo& touch,
      InputEventAckState ack_result) {}
  // Gesture events with unused scroll deltas must be bubbled to ancestors
  // who may consume the delta.
  virtual void BubbleScrollEvent(const blink::WebGestureEvent& event) {}

  // Determines whether the root RenderWidgetHostView (and thus the current
  // page) has focus.
  virtual bool HasFocus();
  // Focuses the root RenderWidgetHostView.
  virtual void FocusRootView() {}

  // Locks the mouse. Returns true if mouse is locked.
  virtual bool LockMouse();

  // Unlocks the mouse if the mouse is locked.
  virtual void UnlockMouse() {}

  const gfx::Rect& viewport_intersection() const {
    return viewport_intersection_rect_;
  }

  virtual bool is_inert() const;
  virtual bool is_hidden() const;

 protected:
  virtual ~FrameConnectorDelegate() {}

  gfx::Rect viewport_intersection_rect_;
};

}  // namespace content

#endif  // CONTENT_BROWSER_RENDERER_HOST_FRAME_CONNECTOR_DELEGATE_H_
