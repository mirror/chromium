// Copyright (c) 2006-2008 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifdef ENABLE_BACKGROUND_TASK
#include "chrome/browser/notifications/balloon_view.h"

#include "base/gfx/gdi_util.h"
#include "chrome/app/theme/theme_resources.h"
#include "chrome/browser/notifications/balloon_contents.h"
#include "chrome/browser/notifications/balloons.h"
#include "chrome/common/gfx/chrome_canvas.h"
#include "chrome/common/notification_details.h"
#include "chrome/common/notification_source.h"
#include "chrome/views/container_win.h"
#include "chrome/views/painter.h"

namespace {
// How many pixels of overlap there is between the shelf top and the
// balloon bottom.
const int kTopMargin = 1;
const int kBottomMargin = 1;
const int kLeftMargin = 1;
const int kRightMargin = 1;
const int kShelfBoarderTopOverlap = 2;

// TODO(levin): Add a shadow for the frame.
const int kLeftShadowWidth = 0;
const int kRightShadowWidth = 0;
const int kTopShadowWidth = 0;
const int kBottomShadowWidth = 0;

// The shelf height for the system default font size.  It is scaled
// with changes in the default font size.
const int kDefaultShelfHeight = 22;
}  // namespace

BalloonView::BalloonView()
    : balloon_(NULL),
      frame_container_(NULL),
      html_container_(NULL),
      method_factory_(this) {
  int shelf_images[9];
  shelf_images[views::ImagePainter::BORDER_TOP_LEFT] =
      IDR_BALLOON_SHELF_TOP_LEFT;
  shelf_images[views::ImagePainter::BORDER_TOP] = IDR_BALLOON_SHELF_TOP_CENTER;
  shelf_images[views::ImagePainter::BORDER_TOP_RIGHT] =
      IDR_BALLOON_SHELF_TOP_RIGHT;
  shelf_images[views::ImagePainter::BORDER_RIGHT] = IDR_BALLOON_SHELF_RIGHT;
  shelf_images[views::ImagePainter::BORDER_BOTTOM_RIGHT] =
      IDR_BALLOON_SHELF_BOTTOM_RIGHT;
  shelf_images[views::ImagePainter::BORDER_BOTTOM] =
      IDR_BALLOON_SHELF_BOTTOM_CENTER;
  shelf_images[views::ImagePainter::BORDER_BOTTOM_LEFT] =
      IDR_BALLOON_SHELF_BOTTOM_LEFT;
  shelf_images[views::ImagePainter::BORDER_LEFT] = IDR_BALLOON_SHELF_LEFT;
  shelf_images[views::ImagePainter::BORDER_CENTER] = IDR_BALLOON_SHELF_CENTER;
  shelf_background_.reset(new views::ImagePainter(shelf_images, true));

  int balloon_images[8];
  balloon_images[views::ImagePainter::BORDER_TOP_LEFT] = IDR_BALLOON_TOP_LEFT;
  balloon_images[views::ImagePainter::BORDER_TOP] = IDR_BALLOON_TOP_CENTER;
  balloon_images[views::ImagePainter::BORDER_TOP_RIGHT] = IDR_BALLOON_TOP_RIGHT;
  balloon_images[views::ImagePainter::BORDER_RIGHT] = IDR_BALLOON_RIGHT;
  balloon_images[views::ImagePainter::BORDER_BOTTOM_RIGHT] =
      IDR_BALLOON_BOTTOM_RIGHT;
  balloon_images[views::ImagePainter::BORDER_BOTTOM] =
      IDR_BALLOON_BOTTOM_CENTER;
  balloon_images[views::ImagePainter::BORDER_BOTTOM_LEFT] =
      IDR_BALLOON_BOTTOM_LEFT;
  balloon_images[views::ImagePainter::BORDER_LEFT] = IDR_BALLOON_LEFT;
  balloon_background_.reset(new views::ImagePainter(balloon_images, false));
}

BalloonView::~BalloonView() {
}

void BalloonView::Close() {
  MessageLoop::current()->PostTask(
        FROM_HERE,
        method_factory_.NewRunnableMethod(&BalloonView::DelayedClose));
}

void BalloonView::DelayedClose() {
  html_contents_->Shutdown();
  html_container_->CloseNow();
  frame_container_->CloseNow();
}

void BalloonView::DidChangeBounds(const gfx::Rect& previous,
                                  const gfx::Rect& current) {
  SizeContentsWindow();
}

void BalloonView::SizeContentsWindow() {
  if (!html_container_ || !frame_container_) {
    return;
  }
  gfx::Rect contents_rect = contents_rectangle();
  html_container_->SetWindowPos(frame_container_->GetHWND(),
                                contents_rect.x(),
                                contents_rect.y(),
                                contents_rect.width(),
                                contents_rect.height(),
                                SWP_NOACTIVATE);

  // Note: System will own the hrgn after we call SetWindowRgn,
  // so we don't need to call DeleteObject() for the mask.
  ::SetWindowRgn(html_container_->GetHWND(),
                 GetContentsMask(contents_rect),
                 false);
}

void BalloonView::Show(Balloon* balloon) {
  balloon_ = balloon;

  SetBounds(balloon->position().x(),
            balloon->position().y(),
            balloon->size().width(),
            balloon->size().height());

  // We have to create two windows: one for the contents and one for the
  // frame.  Why?
  // * The contents is an html window which cannot be a
  //   layered window (because it may have child windows for instance).
  // * The frame is a layered window so that we can have nicely rounded
  //   corners using alpha blending (and we may do other alpha blending
  //   effects).
  // Unfortunately, layered windows cannot have child windows. (Well, they can
  // but the child windows don't render).
  //
  // We carefully keep these two windows in sync to present the illusion of
  // one window to the user.

  html_container_ = new views::ContainerWin();
  html_container_->set_window_style(WS_POPUP);
  html_container_->set_window_ex_style(WS_EX_TOPMOST | WS_EX_TOOLWINDOW);
  gfx::Rect contents_rect = contents_rectangle();
  html_container_->Init(NULL, contents_rect, false);
  html_contents_ = new BalloonContents(balloon);
  html_contents_->set_preferred_size(gfx::Size(10000, 10000));
  html_container_->SetContentsView(html_contents_);

  frame_container_ = new views::ContainerWin();
  frame_container_->set_window_style(WS_POPUP);
  frame_container_->set_window_ex_style(
      WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW);
  gfx::Rect balloon_rect(x(), y(), width(), height());
  frame_container_->Init(NULL, balloon_rect, false);
  frame_container_->SetContentsView(this);

  SizeContentsWindow();
  html_container_->ShowWindow(SW_SHOWNOACTIVATE);
  frame_container_->ShowWindow(SW_SHOWNOACTIVATE);

  NotificationService::current()->AddObserver(
      this, NOTIFY_BALLOON_DISCONNECTED, Source<Balloon>(balloon));
}


HRGN BalloonView::GetContentsMask(gfx::Rect& contents_rect) const {
  // This needs to remove areas that look like the following from each corner:
  //
  //   xx
  //   x

  // Ideally, the frame would alpha blend these pixels on to the balloon
  // contents. However, that is a problem because the frame and contents are
  // separate windows, and the OS may swap in z order (so the contents goes on
  // top of the frame and covers the alpha blended pixels).

  std::vector<gfx::Rect> cutouts;
  // Upper left.
  cutouts.push_back(gfx::Rect(0, 0, 2, 1));
  cutouts.push_back(gfx::Rect(0, 1, 1, 1));

  // Upper right.
  cutouts.push_back(gfx::Rect(contents_rect.width() - 2, 0, 2, 1));
  cutouts.push_back(gfx::Rect(contents_rect.width() - 1, 1, 1, 1));

  // Lower left.
  cutouts.push_back(gfx::Rect(0, contents_rect.height() - 1, 2, 1));
  cutouts.push_back(gfx::Rect(0, contents_rect.height() - 2, 1, 1));

  // Lower right.
  cutouts.push_back(gfx::Rect(contents_rect.width() - 2,
                              contents_rect.height() - 1,
                              2,
                              1));
  cutouts.push_back(gfx::Rect(contents_rect.width() - 1,
                              contents_rect.height() - 2,
                              1,
                              1));

  HRGN hrgn = ::CreateRectRgn(0,
                              0,
                              contents_rect.width(),
                              contents_rect.height());
  gfx::SubtractRectanglesFromRegion(hrgn, cutouts);
  return hrgn;
}

gfx::Point BalloonView::contents_offset() const {
  return gfx::Point(kTopShadowWidth + kTopMargin,
                    kLeftShadowWidth + kLeftMargin);
}

int BalloonView::shelf_height() const {
  // TODO(levin): add scaling here.
  return kDefaultShelfHeight;
}

int BalloonView::frame_width() const {
  return size().width() - kLeftShadowWidth - kRightShadowWidth;
}

int BalloonView::total_frame_height() const {
  return size().height() - kTopShadowWidth - kBottomShadowWidth;
}

int BalloonView::balloon_frame_height() const {
  return total_frame_height() - shelf_height();
}

gfx::Rect BalloonView::contents_rectangle() const {
  if (!frame_container_) {
    return gfx::Rect();
  }
  int contents_width = frame_width() - kLeftMargin - kRightMargin;
  int contents_height = balloon_frame_height() - kTopMargin - kBottomMargin;
  gfx::Point offset = contents_offset();
  CRect frame_rect;
  frame_container_->GetBounds(&frame_rect, true);
  return gfx::Rect(frame_rect.left + offset.x(),
                   frame_rect.top + offset.y(),
                   contents_width,
                   contents_height);
}

void BalloonView::Paint(ChromeCanvas* canvas) {
  DCHECK(canvas);

  int background_width = frame_width();
  int background_height = balloon_frame_height();

  balloon_background_->Paint(background_width, background_height, canvas);

  canvas->save();
  SkScalar y_offset =
      static_cast<SkScalar>(background_height - kShelfBoarderTopOverlap);
  canvas->translate(0, y_offset);
  shelf_background_->Paint(background_width, shelf_height(), canvas);
  canvas->restore();

  View::Paint(canvas);
}

void BalloonView::Observe(NotificationType type,
                          const NotificationSource& source,
                          const NotificationDetails& details) {
  if (type != NOTIFY_BALLOON_DISCONNECTED) {
    NOTREACHED();
    return;
  }
  NotificationService::current()->RemoveObserver(
      this, NOTIFY_BALLOON_DISCONNECTED, Source<Balloon>(balloon_));
  Close();
}

#endif  // ENABLE_BACKGROUND_TASK
