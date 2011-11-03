// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef VIEWS_BUBBLE_BUBBLE_DELEGATE_H_
#define VIEWS_BUBBLE_BUBBLE_DELEGATE_H_
#pragma once

#include "base/gtest_prod_util.h"
#include "ui/base/animation/animation_delegate.h"
#include "views/bubble/bubble_border.h"
#include "views/widget/widget_delegate.h"

namespace ui {
class SlideAnimation;
}  // namespace ui

namespace views {

class BubbleFrameView;

// BubbleDelegateView creates frame and client views for bubble Widgets.
// BubbleDelegateView itself is the client's contents view.
//
///////////////////////////////////////////////////////////////////////////////
class VIEWS_EXPORT BubbleDelegateView : public WidgetDelegateView,
                                        public ui::AnimationDelegate {
 public:
  BubbleDelegateView();
  BubbleDelegateView(const gfx::Point& anchor_point,
                     BubbleBorder::ArrowLocation arrow_location,
                     const SkColor& color);
  virtual ~BubbleDelegateView();

  // Create and initialize the bubble Widget(s) with proper bounds.
  static Widget* CreateBubble(BubbleDelegateView* bubble_delegate,
                              Widget* parent_widget);

  // WidgetDelegate overrides:
  virtual View* GetInitiallyFocusedView() OVERRIDE;
  virtual View* GetContentsView() OVERRIDE;
  virtual NonClientFrameView* CreateNonClientFrameView() OVERRIDE;

  bool close_on_esc() const { return close_on_esc_; }
  void set_close_on_esc(bool close_on_esc) { close_on_esc_ = close_on_esc; }

  // Get the arrow's anchor point in screen space.
  virtual gfx::Point GetAnchorPoint();

  // Get the arrow's location on the bubble.
  virtual BubbleBorder::ArrowLocation GetArrowLocation() const;

  // Get the color used for the background and border.
  virtual SkColor GetColor() const;

  // Show the bubble's widget (and |border_widget_| on Windows).
  void Show();

  // Fade the bubble in or out via Widget transparency.
  // Fade in calls Widget::Show; fade out calls Widget::Close upon completion.
  void StartFade(bool fade_in);

  // Reset fade and opacity of bubble. Restore the opacity of the
  // bubble to the setting before StartFade() was called.
  void ResetFade();

 protected:
  // View overrides:
  virtual bool AcceleratorPressed(const Accelerator& accelerator) OVERRIDE;

  // Perform view initialization on the contents for bubble sizing.
  virtual void Init();

 private:
  FRIEND_TEST_ALL_PREFIXES(BubbleFrameViewBasicTest, NonClientHitTest);
  FRIEND_TEST_ALL_PREFIXES(BubbleDelegateTest, CreateDelegate);

  // ui::AnimationDelegate overrides:
  virtual void AnimationEnded(const ui::Animation* animation);
  virtual void AnimationProgressed(const ui::Animation* animation);

  BubbleFrameView* GetBubbleFrameView() const;

  // Get bubble bounds from the anchor point and client view's preferred size.
  gfx::Rect GetBubbleBounds();

#if defined(OS_WIN) && !defined(USE_AURA)
  // Initialize the border widget needed for Windows native control hosting.
  void InitializeBorderWidget(Widget* parent_widget);

  // Get bounds for the Windows-only widget that hosts the bubble's contents.
  gfx::Rect GetBubbleClientBounds() const;
#endif

  // Fade animation for bubble.
  scoped_ptr<ui::SlideAnimation> fade_animation_;

  // Should this bubble close on the escape key?
  bool close_on_esc_;

  // The screen point where this bubble's arrow is anchored.
  gfx::Point anchor_point_;

  // The arrow's location on the bubble.
  BubbleBorder::ArrowLocation arrow_location_;

  // The background color of the bubble.
  SkColor color_;

  // Original opacity of the bubble.
  int original_opacity_;

  // The widget hosting the border for this bubble (non-Aura Windows only).
  Widget* border_widget_;

  DISALLOW_COPY_AND_ASSIGN(BubbleDelegateView);
};

}  // namespace views

#endif  // VIEWS_BUBBLE_BUBBLE_DELEGATE_H_
