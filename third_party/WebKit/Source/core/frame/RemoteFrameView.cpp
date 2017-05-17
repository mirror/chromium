// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/frame/RemoteFrameView.h"

#include "core/dom/IntersectionObserverEntry.h"
#include "core/frame/FrameView.h"
#include "core/frame/LocalFrame.h"
#include "core/frame/RemoteFrame.h"
#include "core/frame/RemoteFrameClient.h"
#include "core/html/HTMLFrameOwnerElement.h"
#include "core/layout/LayoutView.h"
#include "core/layout/api/LayoutPartItem.h"

namespace blink {

RemoteFrameView::RemoteFrameView(RemoteFrame* remote_frame)
    : remote_frame_(remote_frame), remote_frame_view_state_(kNotAttached) {
  DCHECK(remote_frame);
}

RemoteFrameView::~RemoteFrameView() {}

FrameView* RemoteFrameView::ParentFrameView() const {
  if (remote_frame_view_state_ != kAttached)
    return nullptr;

  Frame* parent_frame = remote_frame_->Tree().Parent();
  if (parent_frame && parent_frame->IsLocalFrame())
    return ToLocalFrame(parent_frame)->View();

  return nullptr;
}

void RemoteFrameView::SetFrameOrPluginState(FrameOrPluginState state) {
  VLOG(1) << "SetFrameOrPluginState " << this << " " << remote_frame_view_state_
          << "->" << state;
  if (VLOG_IS_ON(2))
    base::debug::StackTrace(10).Print();
  switch (state) {
    case kNotAttached:
      DCHECK(remote_frame_view_state_ == kAttached ||
             remote_frame_view_state_ == kDisposed);
      SetParentVisible(false);
      break;

    case kAttached:
      DCHECK(remote_frame_view_state_ == kNotAttached ||
             remote_frame_view_state_ == kDeferred ||
             remote_frame_view_state_ == kDisposed);
      SetParentVisible(true);
      FrameRectsChanged();
      break;

    case kDeferred:
      DCHECK(remote_frame_view_state_ == kNotAttached ||
             remote_frame_view_state_ == kAttached ||
             remote_frame_view_state_ == kDeferred ||
             remote_frame_view_state_ == kDisposed);
      break;
    case kDisposed:
      DCHECK(remote_frame_view_state_ == kAttached ||
             remote_frame_view_state_ == kNotAttached ||
             remote_frame_view_state_ == kDeferred ||
             remote_frame_view_state_ == kDisposed);
      break;
    default:
      NOTREACHED();
  }
  remote_frame_view_state_ = state;
}

RemoteFrameView* RemoteFrameView::Create(RemoteFrame* remote_frame) {
  RemoteFrameView* view = new RemoteFrameView(remote_frame);
  view->Show();
  return view;
}

void RemoteFrameView::UpdateRemoteViewportIntersection() {
  if (!remote_frame_->OwnerLayoutObject())
    return;

  FrameView* local_root_view =
      ToLocalFrame(remote_frame_->Tree().Parent())->LocalFrameRoot().View();
  if (!local_root_view)
    return;

  // Start with rect in remote frame's coordinate space. Then
  // mapToVisualRectInAncestorSpace will move it to the local root's coordinate
  // space and account for any clip from containing elements such as a
  // scrollable div. Passing nullptr as an argument to
  // mapToVisualRectInAncestorSpace causes it to be clipped to the viewport,
  // even if there are RemoteFrame ancestors in the frame tree.
  LayoutRect rect(0, 0, frame_rect_.Width(), frame_rect_.Height());
  rect.Move(remote_frame_->OwnerLayoutObject()->ContentBoxOffset());
  IntRect viewport_intersection;
  if (remote_frame_->OwnerLayoutObject()->MapToVisualRectInAncestorSpace(
          nullptr, rect)) {
    IntRect root_visible_rect = local_root_view->VisibleContentRect();
    IntRect intersected_rect(rect);
    intersected_rect.Intersect(root_visible_rect);
    intersected_rect.Move(-local_root_view->ScrollOffsetInt());

    // Translate the intersection rect from the root frame's coordinate space
    // to the remote frame's coordinate space.
    viewport_intersection = ConvertFromRootFrame(intersected_rect);
  }

  if (viewport_intersection != last_viewport_intersection_) {
    remote_frame_->Client()->UpdateRemoteViewportIntersection(
        viewport_intersection);
  }

  last_viewport_intersection_ = viewport_intersection;
}

void RemoteFrameView::Dispose() {
  SetFrameOrPluginState(kDisposed);

  HTMLFrameOwnerElement* owner_element = remote_frame_->DeprecatedLocalOwner();
  // ownerElement can be null during frame swaps, because the
  // RemoteFrameView is disconnected before detachment.
  if (owner_element && owner_element->OwnedWidget() == this)
    owner_element->SetWidget(nullptr);
}

void RemoteFrameView::InvalidateRect(const IntRect& rect) {
  LayoutPartItem layout_item = remote_frame_->OwnerLayoutItem();
  if (layout_item.IsNull())
    return;

  LayoutRect repaint_rect(rect);
  repaint_rect.Move(layout_item.BorderLeft() + layout_item.PaddingLeft(),
                    layout_item.BorderTop() + layout_item.PaddingTop());
  layout_item.InvalidatePaintRectangle(repaint_rect);
}

void RemoteFrameView::SetFrameRect(const IntRect& frame_rect) {
  if (frame_rect == frame_rect_)
    return;

  frame_rect_ = frame_rect;
  FrameRectsChanged();
}

void RemoteFrameView::FrameRectsChanged() {
  // Update the rect to reflect the position of the frame relative to the
  // containing local frame root. The position of the local root within
  // any remote frames, if any, is accounted for by the embedder.
  IntRect new_rect = frame_rect_;

  // TODO(joelhockey): I don't think it makes sense for this to be called
  // when it is not attached.
  // This gets called in kNotAttached state from
  // SetWidget : UpdateOnWidgetChange : UpdateGeometryInternal :
  // FrameRectsChanged however UpdateOnWidgetChange gets called before the
  // SetParent which makes this move to kAttached state. This is also called in
  // kDeferred state for SetParent(nullptr).
  // kDisposed ?
  DCHECK(remote_frame_view_state_ == kAttached ||
         remote_frame_view_state_ == kNotAttached ||
         remote_frame_view_state_ == kDeferred ||
         remote_frame_view_state_ == kDisposed);
  if (FrameView* parent = ParentFrameView())
    new_rect = parent->ConvertToRootFrame(parent->ContentsToFrame(new_rect));
  remote_frame_->Client()->FrameRectsChanged(new_rect);
}

void RemoteFrameView::Hide() {
  self_visible_ = false;
  remote_frame_->Client()->VisibilityChanged(false);
}

void RemoteFrameView::Show() {
  self_visible_ = true;
  remote_frame_->Client()->VisibilityChanged(true);
}

void RemoteFrameView::SetParentVisible(bool visible) {
  if (parent_visible_ == visible)
    return;

  parent_visible_ = visible;
  if (!self_visible_)
    return;

  remote_frame_->Client()->VisibilityChanged(self_visible_ && parent_visible_);
}

IntRect RemoteFrameView::ConvertFromRootFrame(
    const IntRect& rect_in_root_frame) const {
  // TODO(joelhockey): I think this should only be called when in state
  // kAttached. However, it gets called in state kDisposed from
  // content_browsertest
  // --gtest_filter=SitePerProcessBrowserTest.SubframePendingAndBackToSameSiteInstance
  // Called in state kDeferred from
  // browser_tests
  // --gtest_filter=WebViewTests/WebViewTest.Shim_TestDisplayBlock/1
  DCHECK(remote_frame_view_state_ == kAttached ||
         remote_frame_view_state_ == kDeferred ||
         remote_frame_view_state_ == kDisposed);
  if (FrameView* parent = ParentFrameView()) {
    IntRect parent_rect = parent->ConvertFromRootFrame(rect_in_root_frame);
    parent_rect.SetLocation(
        parent->ConvertSelfToChild(*this, parent_rect.Location()));
    return parent_rect;
  }
  return rect_in_root_frame;
}

DEFINE_TRACE(RemoteFrameView) {
  visitor->Trace(remote_frame_);
}

}  // namespace blink
