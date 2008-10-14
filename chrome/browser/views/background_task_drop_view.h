// Copyright (c) 2006-2008 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_VIEWS_BACKGROUND_TASK_DROP_VIEW_H__
#define CHROME_BROWSER_VIEWS_BACKGROUND_TASK_DROP_VIEW_H__

#if ENABLE_BACKGROUND_TASK == 1
#include "base/basictypes.h"
#include "base/gfx/size.h"
#include "chrome/common/animation.h"
#include "chrome/common/gfx/chrome_font.h"
#include "chrome/views/view.h"

namespace ChromeViews {
class HWNDViewContainer;
}
class SlideAnimation;

/////////////////////////////////////////////////////////////////////////////
//
// BackgroundTaskDropView class
// The BackgroundTaskDrop View is a top level window that is displayed
// when an appropriate tag is dragged from the browser.  If the
// item is dropped on this window, then it is started as a background task.
//
/////////////////////////////////////////////////////////////////////////////
class BackgroundTaskDropView : public ChromeViews::View,
                               public AnimationDelegate {
 public:
  BackgroundTaskDropView(const std::wstring& site);
  virtual ~BackgroundTaskDropView();

 private:
  // Overridden from ChromeViews::View:
  virtual void Paint(ChromeCanvas* canvas);
  virtual void GetPreferredSize(CSize* out);
  virtual void OnMouseEntered(const ChromeViews::MouseEvent& event);
  virtual void OnMouseExited(const ChromeViews::MouseEvent& event);

  // Overridden from AnimationDelegate:
  virtual void AnimationProgressed(const Animation* animation);
  virtual void AnimationCanceled(const Animation* animation);
  virtual void AnimationEnded(const Animation* animation);

  // The window that contains the BackgroundTaskDropView.
  ChromeViews::HWNDViewContainer* container_;

  // The dimensions of the drop window.
  gfx::Size contents_size_;

  // Hover animation.
  scoped_ptr<SlideAnimation> hover_animation_;

  // Bitmaps for the drop box in the hover state.
  SkBitmap* drop_box_hover_top_left_;
  SkBitmap* drop_box_hover_top_center_;
  SkBitmap* drop_box_hover_left_;
  SkBitmap* drop_box_hover_center_;

  // Bitmaps for the drop box in the inactive state.
  SkBitmap* drop_box_inactive_top_left_;
  SkBitmap* drop_box_inactive_top_center_;
  SkBitmap* drop_box_inactive_left_;
  SkBitmap* drop_box_inactive_center_;

  DISALLOW_EVIL_CONSTRUCTORS(BackgroundTaskDropView);
};

#endif  // ENABLE_BACKGROUND_TASK == 1
#endif  // CHROME_BROWSER_VIEWS_BACKGROUND_TASK_DROP_VIEW_H__
