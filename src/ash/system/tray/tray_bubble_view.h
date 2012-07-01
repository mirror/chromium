// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SYSTEM_TRAY_TRAY_BUBBLE_VIEW_H_
#define ASH_SYSTEM_TRAY_TRAY_BUBBLE_VIEW_H_
#pragma once

#include "ui/aura/event_filter.h"
#include "ui/views/bubble/bubble_delegate.h"

namespace aura {
class LocatedEvent;
}

namespace views {
class View;
class Widget;
}

namespace ash {
namespace internal {

// Specialized bubble view for status area tray bubbles.
// Mostly this handles custom anchor location and arrow and border rendering.
class TrayBubbleView : public views::BubbleDelegateView {
 public:
  class Host : public aura::EventFilter {
   public:
    Host();
    virtual ~Host();

    void InitializeHost(views::Widget* widget, views::View* tray_view);

    virtual void BubbleViewDestroyed() = 0;
    virtual gfx::Rect GetAnchorRect() const = 0;
    virtual void OnMouseEnteredView() = 0;
    virtual void OnMouseExitedView() = 0;
    virtual void OnClickedOutsideView() = 0;

    // Overridden from aura::EventFilter.
    virtual bool PreHandleKeyEvent(aura::Window* target,
                                   aura::KeyEvent* event) OVERRIDE;
    virtual bool PreHandleMouseEvent(aura::Window* target,
                                     aura::MouseEvent* event) OVERRIDE;
    virtual ui::TouchStatus PreHandleTouchEvent(
        aura::Window* target,
        aura::TouchEvent* event) OVERRIDE;
    virtual ui::GestureStatus PreHandleGestureEvent(
        aura::Window* target,
        aura::GestureEvent* event) OVERRIDE;

   private:
    void ProcessLocatedEvent(const aura::LocatedEvent& event);

    views::Widget* widget_;
    views::View* tray_view_;

    DISALLOW_COPY_AND_ASSIGN(Host);
  };

  TrayBubbleView(views::View* anchor,
                 views::BubbleBorder::ArrowLocation arrow_location,
                 Host* host,
                 bool can_activate,
                 int bubble_width);
  virtual ~TrayBubbleView();

  // Creates a bubble border with the specified arrow offset.
  void SetBubbleBorder(int arrow_offset);

  // Called whenever the bubble anchor location may have moved.
  void UpdateAnchor();

  // Sets the maximum bubble height and resizes the bubble.
  void SetMaxHeight(int height);

  // Called when the host is destroyed.
  void reset_host() { host_ = NULL; }

  // Overridden from views::WidgetDelegate.
  virtual bool CanActivate() const OVERRIDE;

  // Overridden from views::BubbleDelegateView.
  virtual gfx::Rect GetAnchorRect() OVERRIDE;

  // Overridden from views::View.
  virtual gfx::Size GetPreferredSize() OVERRIDE;
  virtual void OnMouseEntered(const views::MouseEvent& event) OVERRIDE;
  virtual void OnMouseExited(const views::MouseEvent& event) OVERRIDE;
  virtual void GetAccessibleState(ui::AccessibleViewState* state) OVERRIDE;

 protected:
  // Overridden from views::BubbleDelegateView.
  virtual void Init() OVERRIDE;
  virtual gfx::Rect GetBubbleBounds() OVERRIDE;

  // Overridden from views::View.
  virtual void ChildPreferredSizeChanged(View* child) OVERRIDE;
  virtual void ViewHierarchyChanged(bool is_add,
                                    views::View* parent,
                                    views::View* child) OVERRIDE;

 private:
  Host* host_;
  bool can_activate_;
  int max_height_;
  int bubble_width_;

  DISALLOW_COPY_AND_ASSIGN(TrayBubbleView);
};

}  // namespace internal
}  // namespace ash

#endif  // ASH_SYSTEM_TRAY_TRAY_BUBBLE_VIEW_H_
