// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_RENDERER_HOST_RENDER_WIDGET_HOST_VIEW_DFB_H_
#define CONTENT_BROWSER_RENDERER_HOST_RENDER_WIDGET_HOST_VIEW_DFB_H_

#include <string>
#include <vector>

#include "base/memory/scoped_ptr.h"
#include "base/time.h"
#include "content/browser/accessibility/browser_accessibility_manager.h"
#include "content/browser/renderer_host/render_widget_host_view_base.h"
#include "content/common/content_export.h"
#include "ui/base/animation/animation_delegate.h"
#include "ui/base/animation/slide_animation.h"
#include "ui/gfx/native_widget_types.h"
#include "ui/gfx/point.h"
#include "ui/gfx/rect.h"
#include "webkit/glue/webcursor.h"

namespace content {
class RenderWidgetHost;
class RenderWidgetHostImpl;
struct NativeWebKeyboardEvent;

// -----------------------------------------------------------------------------
// See comments in render_widget_host_view.h about this class and its members.
// -----------------------------------------------------------------------------
class CONTENT_EXPORT RenderWidgetHostViewDfb : public RenderWidgetHostViewBase {
 public:
  virtual ~RenderWidgetHostViewDfb();

  // RenderWidgetHostView implementation.
  virtual void InitAsChild(gfx::NativeView parent_view) OVERRIDE;
  virtual RenderWidgetHost* GetRenderWidgetHost() const OVERRIDE;
  virtual void SetSize(const gfx::Size& size) OVERRIDE;
  virtual void SetBounds(const gfx::Rect& rect) OVERRIDE;
  virtual gfx::NativeView GetNativeView() const OVERRIDE;
  virtual gfx::NativeViewId GetNativeViewId() const OVERRIDE;
  virtual gfx::NativeViewAccessible GetNativeViewAccessible() OVERRIDE;
  virtual bool HasFocus() const OVERRIDE;
  virtual bool IsSurfaceAvailableForCopy() const OVERRIDE;
  virtual void Show() OVERRIDE;
  virtual void Hide() OVERRIDE;
  virtual bool IsShowing() OVERRIDE;
  virtual gfx::Rect GetViewBounds() const OVERRIDE;
  virtual void SetBackground(const SkBitmap& background) OVERRIDE;

  // RenderWidgetHostViewPort implementation.
  virtual void InitAsPopup(RenderWidgetHostView* parent_host_view,
                           const gfx::Rect& pos) OVERRIDE;
  virtual void InitAsFullscreen(
      RenderWidgetHostView* reference_host_view) OVERRIDE;
  virtual void WasShown() OVERRIDE;
  virtual void WasHidden() OVERRIDE;
  virtual void MovePluginWindows(
      const gfx::Vector2d& scroll_offset,
      const std::vector<webkit::npapi::WebPluginGeometry>& moves) OVERRIDE;
  virtual void Focus() OVERRIDE;
  virtual void Blur() OVERRIDE;
  virtual void UpdateCursor(const WebCursor& cursor) OVERRIDE;
  virtual void SetIsLoading(bool is_loading) OVERRIDE;
  virtual void TextInputStateChanged(
      const ViewHostMsg_TextInputState_Params& params) OVERRIDE;
  virtual void ImeCancelComposition() OVERRIDE;
  virtual void ImeCompositionRangeChanged(
      const ui::Range& range,
      const std::vector<gfx::Rect>& character_bounds) OVERRIDE;
  virtual void DidUpdateBackingStore(
      const gfx::Rect& scroll_rect,
      const gfx::Vector2d& scroll_delta,
      const std::vector<gfx::Rect>& copy_rects) OVERRIDE;
  virtual void RenderViewGone(base::TerminationStatus status,
                              int error_code) OVERRIDE;
  virtual void Destroy() OVERRIDE;
  virtual void WillDestroyRenderWidget(RenderWidgetHost* rwh) {}
  virtual void SetTooltipText(const string16& tooltip_text) OVERRIDE;
  virtual void SelectionBoundsChanged(
      const ViewHostMsg_SelectionBounds_Params& params) OVERRIDE;
  virtual void ScrollOffsetChanged() OVERRIDE;
  virtual BackingStore* AllocBackingStore(const gfx::Size& size) OVERRIDE;
  virtual void CopyFromCompositingSurface(
      const gfx::Rect& src_subrect,
      const gfx::Size& dst_size,
      const base::Callback<void(bool, const SkBitmap&)>& callback) OVERRIDE;
  virtual void CopyFromCompositingSurfaceToVideoFrame(
      const gfx::Rect& src_subrect,
      const scoped_refptr<media::VideoFrame>& target,
      const base::Callback<void(bool)>& callback) OVERRIDE;
  virtual bool CanCopyToVideoFrame() const OVERRIDE;
  virtual void OnAcceleratedCompositingStateChange() OVERRIDE;
  virtual void AcceleratedSurfaceBuffersSwapped(
      const GpuHostMsg_AcceleratedSurfaceBuffersSwapped_Params& params,
      int gpu_host_id) OVERRIDE;
  virtual void AcceleratedSurfacePostSubBuffer(
      const GpuHostMsg_AcceleratedSurfacePostSubBuffer_Params& params,
      int gpu_host_id) OVERRIDE;
  virtual void AcceleratedSurfaceSuspend() OVERRIDE;
  virtual void AcceleratedSurfaceRelease() OVERRIDE;
  virtual bool HasAcceleratedSurface(const gfx::Size& desired_size) OVERRIDE;
  virtual void SetHasHorizontalScrollbar(
      bool has_horizontal_scrollbar) OVERRIDE;
  virtual void SetScrollOffsetPinning(
      bool is_pinned_to_left, bool is_pinned_to_right) OVERRIDE;
  virtual void GetScreenInfo(WebKit::WebScreenInfo* results) OVERRIDE;
  virtual gfx::Rect GetBoundsInRootWindow() OVERRIDE;
  virtual gfx::GLSurfaceHandle GetCompositingSurface() OVERRIDE;
  virtual bool LockMouse() OVERRIDE;
  virtual void UnlockMouse() OVERRIDE;
  virtual void OnAccessibilityNotifications(
      const std::vector<AccessibilityHostMsg_NotificationParams>& params)
      OVERRIDE;

  // Mouse events always provide a movementX/Y which needs to be computed.
  // Also, mouse lock requires knowledge of last unlocked cursor coordinates.
  // State is stored on the host view to do this, and the mouse event modified.
  void ModifyEventMovementAndCoords(WebKit::WebMouseEvent* event);

  void Paint(const gfx::Rect&);

 protected:
  friend class RenderWidgetHostView;

  // Should construct only via RenderWidgetHostView::CreateViewForWidget.
  explicit RenderWidgetHostViewDfb(RenderWidgetHost* widget);

 private:
  // Update the display cursor for the render view.
  void ShowCurrentCursor();

  // The model object.
  RenderWidgetHostImpl* host_;

  // This is true when we are currently painting and thus should handle extra
  // paint requests by expanding the invalid rect rather than actually
  // painting.
  bool about_to_validate_and_paint_;

  // This is the rectangle which we'll paint.
  gfx::Rect invalid_rect_;

  // Whether or not this widget is hidden.
  bool is_hidden_;

  // Whether we are currently loading.
  bool is_loading_;

  // The cursor for the page. This is passed up from the renderer.
  WebCursor current_cursor_;

  // The time at which this view started displaying white pixels as a result of
  // not having anything to paint (empty backing store from renderer). This
  // value returns true for is_null() if we are not recording whiteout times.
  base::TimeTicks whiteout_start_time_;

  // The time it took after this view was selected for it to be fully painted.
  base::TimeTicks web_contents_switch_paint_time_;

  // We ignore the first mouse release on popups so the popup will remain open.
  bool is_popup_first_mouse_release_;

  // Is the widget fullscreen?
  bool is_fullscreen_;

  // Has the window ever been marked active? Only valid for fullscreen or
  // popup windows.
  bool made_active_;

  // Used to record the last position of the mouse.
  // While the mouse is locked, they store the last known position just as mouse
  // lock was entered.
  // Relative to the upper-left corner of the view.
  gfx::Point unlocked_mouse_position_;
  // Relative to the upper-left corner of the screen.
  gfx::Point unlocked_global_mouse_position_;
  // Last hidden cursor position. Relative to screen.
  gfx::Point global_mouse_position_;
  // Indicates when mouse motion is valid after the widget has moved.
  bool mouse_has_been_warped_to_new_center_;
  // Indicates the cursor has been warped to the unlocked position,
  // but a move event has not yet been received for it there.
  bool mouse_is_being_warped_to_unlocked_position_;

  // The size that we want the renderer to be.  We keep this in a separate
  // variable because resizing in GTK+ is async.
  gfx::Size requested_size_;

  // The latest reported center of the widget, use GetWidgetCenter() to access.
  gfx::Point widget_center_;
  // If the window moves the widget_center will not be valid until we recompute.
  bool widget_center_valid_;

  // The number of times the user has dragged against horizontal edge  of the
  // monitor (if the widget is aligned with that edge). Negative values
  // indicate the left edge, positive the right.
  int dragged_at_horizontal_edge_;

  // The number of times the user has dragged against vertical edge  of the
  // monitor (if the widget is aligned with that edge). Negative values
  // indicate the top edge, positive the bottom.
  int dragged_at_vertical_edge_;
};

}  // namespace content

#endif  // CHROME_BROWSER_RENDERER_HOST_RENDER_WIDGET_HOST_VIEW_DFB_H_
