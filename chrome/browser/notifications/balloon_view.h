// Copyright (c) 2006-2008 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Draws the view for the balloons.

#ifndef CHROME_BROWSER_NOTIFICATIONS_BALLOON_VIEW_H_
#define CHROME_BROWSER_NOTIFICATIONS_BALLOON_VIEW_H_

#ifdef ENABLE_BACKGROUND_TASK

#include "base/basictypes.h"
#include "base/gfx/point.h"
#include "base/gfx/rect.h"
#include "base/gfx/size.h"
#include "base/scoped_ptr.h"
#include "chrome/views/view.h"

namespace views {
class ContainerWin;
class ImagePainter;
}  // namespace views

class Balloon;

class BalloonView : public views::View {
 public:
  BalloonView();
  ~BalloonView();
  void Show(const gfx::Point& upper_left, Balloon* balloon);

 private:
  // Overridden from views::View.
  virtual void Paint(ChromeCanvas* canvas);
  virtual void DidChangeBounds(const gfx::Rect& previous,
                               const gfx::Rect& current);
  virtual gfx::Size GetPreferredSize() {
    return gfx::Size(1000, 1000);
  }

  // How to mask the balloon contents to fit within the frame.
  // The caller is responsible for deleting the returned object.
  HRGN GetContentsMask(gfx::Rect& contents_rect) const;

  // Adjust the contents window size to be appropriate for the frame.
  void SizeContentsWindow();

  // The height of the balloon's shelf.
  // The shelf is where is close button is located.
  int shelf_height() const;

  // The width of the frame (not including any shadow).
  int frame_width() const;

  // The height of the frame (not including any shadow).
  int total_frame_height() const;

  // The height of the part of the frame around the balloon.
  int balloon_frame_height() const;

  // Where the balloon contents should be placed with respect to the top left
  // of the frame.
  gfx::Point contents_offset() const;

  // Where the balloon contents should be in screen coordinates.
  gfx::Rect contents_rectangle() const;

  // The window that contains the BalloonFrame.
  views::ContainerWin* frame_container_;

  // The window that contains the BalloonContents.
  views::ContainerWin* html_container_;

  scoped_ptr<views::ImagePainter> shelf_background_;
  scoped_ptr<views::ImagePainter> balloon_background_;

  DISALLOW_COPY_AND_ASSIGN(BalloonView);
};

#endif  // ENABLE_BACKGROUND_TASK

#endif  // CHROME_BROWSER_NOTIFICATIONS_BALLOON_VIEW_H_
