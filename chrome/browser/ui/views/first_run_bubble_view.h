// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_FIRST_RUN_BUBBLE_VIEW_H_
#define CHROME_BROWSER_UI_VIEWS_FIRST_RUN_BUBBLE_VIEW_H_

#include <memory>

#include "base/macros.h"
#include "ui/events/event.h"
#include "ui/views/bubble/bubble_dialog_delegate.h"
#include "ui/views/controls/link_listener.h"

namespace views {
class EventMonitor;
}

class Browser;

class FirstRunBubbleView : public views::BubbleDialogDelegateView,
                           public views::LinkListener {
 public:
  explicit FirstRunBubbleView(Browser* browser);
  ~FirstRunBubbleView() override;

  void UpdateAnchors(views::View* anchor_view,
                     const gfx::Point& anchor_point,
                     gfx::NativeWindow parent_window);

 protected:
  // views::BubbleDialogDelegateView overrides:
  void Init() override;
  int GetDialogButtons() const override;

 private:
  // This class observes keyboard events, mouse clicks and touch down events
  // targeted towards the anchor widget and dismisses the first run bubble
  // accordingly.
  class FirstRunBubbleCloser : public ui::EventHandler {
   public:
    FirstRunBubbleCloser(FirstRunBubbleView* bubble, gfx::NativeWindow parent);
    ~FirstRunBubbleCloser() override;

    // ui::EventHandler overrides.
    void OnKeyEvent(ui::KeyEvent* event) override;
    void OnMouseEvent(ui::MouseEvent* event) override;
    void OnGestureEvent(ui::GestureEvent* event) override;

   private:
    void CloseBubble();

    // The bubble instance.
    FirstRunBubbleView* bubble_;

    std::unique_ptr<views::EventMonitor> event_monitor_;

    DISALLOW_COPY_AND_ASSIGN(FirstRunBubbleCloser);
  };

  // views::LinkListener overrides:
  void LinkClicked(views::Link* source, int event_flags) override;

  Browser* browser_;
  std::unique_ptr<FirstRunBubbleCloser> bubble_closer_;

  DISALLOW_COPY_AND_ASSIGN(FirstRunBubbleView);
};

#endif  // CHROME_BROWSER_UI_VIEWS_FIRST_RUN_BUBBLE_VIEW_H_
