// Copyright (c) 2006-2008 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifdef ENABLE_BACKGROUND_TASK
#include "chrome/browser/views/background_task_drop_view.h"

#include "base/gfx/image_operations.h"
#include "base/string_util.h"
#include "chrome/app/theme/theme_resources.h"
#include "chrome/browser/background_task_manager.h"
#include "chrome/browser/background_task/bb_drag_data.h"
#include "chrome/browser/profile.h"
#include "chrome/browser/web_contents.h"
#include "chrome/common/drag_drop_types.h"
#include "chrome/common/gfx/chrome_canvas.h"
#include "chrome/common/gfx/chrome_font.h"
#include "chrome/common/resource_bundle.h"
#include "chrome/common/slide_animation.h"
#include "chrome/views/container_win.h"
#include "chrome/views/event.h"
#include "chrome/views/label.h"

const int kTextMargin = 10;
const int kMinTextWidth = 40;

namespace {
// Returns the top, left, and width for the systray.
// The height is always 0.
gfx::Rect GetSysTrayLocation() {
  RECT work_area = { 0 };
  BOOL r = ::SystemParametersInfo(SPI_GETWORKAREA, 0, &work_area, 0);
  DCHECK(r);

  const int kDefaultWidth = 150;  // Arbitrary number.
  gfx::Rect sys_tray_location(work_area.right - kDefaultWidth,
                              work_area.bottom,
                              kDefaultWidth,
                              0);

  // Find the systray so that we can use it to size the box.
  WINDOWINFO win_info = { 0 };
  win_info.cbSize = sizeof(win_info);
  HWND sys_tray_parent = ::FindWindow(L"Shell_TrayWnd", NULL);
  if (sys_tray_parent) {
    HWND sys_tray = ::FindWindowEx(sys_tray_parent, NULL,
                                   L"TrayNotifyWnd", NULL);
    if (sys_tray) {
      r = ::GetWindowInfo(sys_tray, &win_info);
      DCHECK(r);
      sys_tray_location.set_x(win_info.rcWindow.left);
      sys_tray_location.set_width(
          win_info.rcWindow.right - win_info.rcWindow.left);
    }
  }
  return sys_tray_location;
}
}

// TODO(levin): Right now this assumes that the taskbar is at the bottom
// of the screen.  This should be fixed to allow to all locations of
// the taskbar.  Also, this has to be tested on Vista to ensure that it works
// on that platform as well.

BackgroundTaskDropView::BackgroundTaskDropView(WebContents* source,
                                               const std::wstring& site)
    : source_(source),
      container_(NULL) {
  hover_animation_.reset(new SlideAnimation(this));
  SetParentOwned(false);

  ResourceBundle& rb = ResourceBundle::GetSharedInstance();
  // Load the bitmaps.
  drop_box_hover_top_left_ = rb.GetBitmapNamed(IDR_DROP_BOX_HOVER_TOP_LEFT);
  drop_box_hover_top_center_ = rb.GetBitmapNamed(IDR_DROP_BOX_HOVER_TOP_CENTER);
  drop_box_hover_left_ = rb.GetBitmapNamed(IDR_DROP_BOX_HOVER_LEFT);
  drop_box_hover_center_ = rb.GetBitmapNamed(IDR_DROP_BOX_HOVER_CENTER);
  drop_box_inactive_top_left_ =
      rb.GetBitmapNamed(IDR_DROP_BOX_INACTIVE_TOP_LEFT);
  drop_box_inactive_top_center_ =
      rb.GetBitmapNamed(IDR_DROP_BOX_INACTIVE_TOP_CENTER);
  drop_box_inactive_left_ = rb.GetBitmapNamed(IDR_DROP_BOX_INACTIVE_LEFT);
  drop_box_inactive_center_ = rb.GetBitmapNamed(IDR_DROP_BOX_INACTIVE_CENTER);

  // TODO(levin): Put in this string in resources to allow for localization.
  std::wstring text = L"Drag here to allow\n$1\nto run in the background.";
  text = ReplaceStringPlaceholders(text, site, NULL);

  gfx::Rect systray = GetSysTrayLocation();

  // Set-up the drop box text.
  views::Label* drop_box_text = new views::Label(text);
  drop_box_text->SetMultiLine(true);
  drop_box_text->SetEnabled(false);
  drop_box_text->SetColor(SK_ColorWHITE);
  int width = std::max<int>(systray.width(),
                            2 * kTextMargin + kMinTextWidth);
  int height = drop_box_text->GetHeightForWidth(width - 2 * kTextMargin);
  AddChildView(drop_box_text);
  drop_box_text->SetBounds(kTextMargin, kTextMargin,
                           width - 2 * kTextMargin, height);
  contents_size_.SetSize(width, height + 2 * kTextMargin);

  // Create the only real window to enable everything to be displayed.
  container_ = new views::ContainerWin();
  container_->set_window_style(WS_POPUP);
  container_->set_window_ex_style(
      WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW);
  gfx::Rect drop_box_position(systray.x(),
                              systray.y() - contents_size_.height(),
                              contents_size_.width(),
                              contents_size_.height());
  container_->Init(NULL, drop_box_position, false);
  container_->SetContentsView(this);
  container_->ShowWindow(SW_SHOWNOACTIVATE);
}

BackgroundTaskDropView::~BackgroundTaskDropView() {
  GetParent()->RemoveChildView(this);
  container_->Close();
}

////////////////////////////////////////////////////////////////////////////////
// BackgroundTaskDropView, ChromeViews::View overrides:

void BackgroundTaskDropView::Paint(ChromeCanvas* canvas) {
  DCHECK(canvas);

  // Create the blended bitmaps.
  double opacity = hover_animation_->GetCurrentValue();
  SkBitmap top_left = gfx::ImageOperations::CreateBlendedBitmap(
      *drop_box_inactive_top_left_,
      *drop_box_hover_top_left_,
      opacity);
  SkBitmap top_center = gfx::ImageOperations::CreateBlendedBitmap(
      *drop_box_inactive_top_center_,
      *drop_box_hover_top_center_,
      opacity);
  SkBitmap left = gfx::ImageOperations::CreateBlendedBitmap(
      *drop_box_inactive_left_,
      *drop_box_hover_left_,
      opacity);
  SkBitmap center = gfx::ImageOperations::CreateBlendedBitmap(
      *drop_box_inactive_center_,
      *drop_box_hover_center_,
      opacity);

  // Draw the image.
  //
  //    top left | top center
  //    ---------------------
  //    left     | center
  canvas->DrawBitmapInt(top_left, 0, 0, top_left.width(), top_left.height(),
                        0, 0, top_left.width(), top_left.height(), false);
  canvas->DrawBitmapInt(top_center, 0, 0, top_center.width(),
                        top_center.height(), top_left.width(), 0,
                        width() - top_left.width(), top_left.height(), false);
  canvas->DrawBitmapInt(left, 0, 0, left.width(), left.height(), 0,
                        top_left.height(), left.width(),
                        height() - top_left.height(), false);
  canvas->DrawBitmapInt(center, 0, 0, center.width(), center.height(),
                        left.width(), top_left.height(), width() - left.width(),
                        height() - top_left.height(), false);

  // Paint any contained views (like the text).
  View::Paint(canvas);
}


void BackgroundTaskDropView::GetPreferredSize(CSize* out) {
  DCHECK(out);
  *out = CSize(contents_size_.width(), contents_size_.height());
}

bool BackgroundTaskDropView::CanDrop(const OSExchangeData& data) {
  BbDragData bb_data;
  return bb_data.Read(data);
}

void BackgroundTaskDropView::OnDragEntered(const views::DropTargetEvent& event) {
  hover_animation_->SetTweenType(SlideAnimation::EASE_IN);
  hover_animation_->Show();
}

int BackgroundTaskDropView::OnDragUpdated(const views::DropTargetEvent& event) {
  BbDragData bb_data;
  if (!bb_data.Read(event.GetData()))
    return DragDropTypes::DRAG_NONE;

  return DragDropTypes::DRAG_LINK;
}

void BackgroundTaskDropView::OnDragExited() {
  hover_animation_->SetTweenType(SlideAnimation::EASE_OUT);
  hover_animation_->Hide();
}

int BackgroundTaskDropView::OnPerformDrop(const views::DropTargetEvent& event) {
  BbDragData bb_data;
  if (!bb_data.Read(event.GetData()))
    return DragDropTypes::DRAG_NONE;

  BackgroundTaskManager* background_task_manager =
      source_->profile()->GetBackgroundTaskManager();
  if (background_task_manager) {
    std::wstring url = UTF8ToWide(bb_data.url().spec());
    if (background_task_manager->RegisterTask(
            source_,
            bb_data.title(),
            url,
            START_BACKGROUND_TASK_ON_BROWSER_LAUNCH)) {
      background_task_manager->StartTask(source_, bb_data.title());
    }
  }

  return DragDropTypes::DRAG_LINK;
}

///////////////////////////////////////////////////////////////////////////////
// BackgroundTaskDropView, AnimationDelegate implementation:
//
// Allows the box to transition from the inactive state to the hover state.
void BackgroundTaskDropView::AnimationProgressed(const Animation* animation) {
  SchedulePaint();
}

void BackgroundTaskDropView::AnimationCanceled(const Animation* animation) {
  AnimationEnded(animation);
}

void BackgroundTaskDropView::AnimationEnded(const Animation* animation) {
  SchedulePaint();
}

#endif  // ENABLE_BACKGROUND_TASK