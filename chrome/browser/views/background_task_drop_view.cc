// Copyright (c) 2006-2008 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifdef ENABLE_BACKGROUND_TASK
#include "chrome/browser/views/background_task_drop_view.h"

#include "base/gfx/image_operations.h"
#include "base/string_util.h"
#include "chrome/app/chrome_dll_resource.h"
#include "chrome/app/theme/theme_resources.h"
#include "chrome/browser/background_task/background_task_manager.h"
#include "chrome/browser/background_task/bb_drag_data.h"
#include "chrome/browser/profile.h"
#include "chrome/browser/web_contents.h"
#include "chrome/common/drag_drop_types.h"
#include "chrome/common/resource_bundle.h"
#include "chrome/common/slide_animation.h"
#include "chrome/common/throb_animation.h"
#include "chrome/common/gfx/chrome_canvas.h"
#include "chrome/common/gfx/chrome_font.h"
#include "chrome/common/gfx/icon_util.h"
#include "chrome/views/container_win.h"
#include "chrome/views/border.h"
#include "chrome/views/event.h"
#include "chrome/views/label.h"

namespace {
const int kTextMargin = 10;
const int kBitmapMargin = 10;
const int kMaxTextWidth = 400;

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

// Draw a nine grid at a given offset (x_offset, y_offset).
//
//   canvas - what to draw the bitmap into.
//   bitmaps - the bitmap for each nine grid section.
//             Entries may be NULL to omit drawing that section.
//   x_offset - where in the canvas to start the left edge.
//   y_offset - where in the canvas to start the top edge.
//   widths - the width of each bitmap
//   height - the height of each bitmap
void DrawNineGrid(ChromeCanvas* canvas,
                  SkBitmap *(bitmaps[3][3]),
                  int x_offset,
                  int y_offset,
                  int widths[3],
                  int heights[3]) {
  DCHECK(canvas);
  int top_offset = y_offset;
  int height = 0;
  for (int y = 0; y < 3; ++y) {
    height = heights[y];
    int left_offset = x_offset;
    int width = 0;
    for (int x = 0; x < 3; ++x) {
      width = widths[x];

      SkBitmap* bitmap = bitmaps[y][x];
      if (bitmap) {
        canvas->DrawBitmapInt(*bitmap,
                              0, 0, bitmap->width(), bitmap->height(),
                              left_offset, top_offset, width, height,
                              false);
      }
      left_offset += width;
    }
    top_offset += height;
  }
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
  glow_animation_.reset(new ThrobAnimation(this));
  glow_animation_->SetThrobDuration(500);
  glow_animation_->SetTweenType(SlideAnimation::EASE_IN_OUT);
  // Do the glow throbbing for a long time (or until the user
  // finds the drop target).
  glow_animation_->StartThrobbing(1000);
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

  glow_top_left_ = rb.GetBitmapNamed(IDR_DROP_BOX_GLOW_TOP_LEFT);
  glow_top_ = rb.GetBitmapNamed(IDR_DROP_BOX_GLOW_TOP);
  glow_left_ = rb.GetBitmapNamed(IDR_DROP_BOX_GLOW_LEFT);

  // TODO(levin): Move this into ResourceBundle and get the
  // same code in chrome/browser/task_manager_resource_providers.cc
  // to use that function.
  HICON icon = ::LoadIcon(_AtlBaseModule.GetResourceInstance(),
                          MAKEINTRESOURCE(IDR_MAINFRAME));
  if (icon) {
    ICONINFO icon_info = {0};
    BITMAP bitmap_info = {0};

    GetIconInfo(icon, &icon_info);
    GetObject(icon_info.hbmMask, sizeof(bitmap_info), &bitmap_info);

    gfx::Size icon_size(bitmap_info.bmWidth, bitmap_info.bmHeight);
    product_logo_ = IconUtil::CreateSkBitmapFromHICON(icon, icon_size);
  }

  // TODO(levin): Put in this string in resources to allow for localization.
  std::wstring text = L"Drag here to allow\n$1\nto run in the background.";
  text = ReplaceStringPlaceholders(text, site, NULL);

  gfx::Rect systray = GetSysTrayLocation();

  // Set-up the drop box text.
  //
  //  The layout looks like this:
  // +------------------------------------------------------------------------+
  // | glow   |    glow                                                       |
  // | -----------------------------------------------------------------------|
  // |        |          bitmap                    text margin                |
  // |        |          margin              +----------------------------+ t |
  // |        |      +--------------+        |                            | x |
  // | glow   |      |              |        |   "Drag here to allow"     | t |
  // |        |bitmap|     icon     | text   |      Security Origin       |   |
  // |        |margin| (centered    | margin | "to run in the background."| m |
  // |        |      |  vertically) |        |                            | a |
  // |        |      |              |        |                            | r |
  // |        |      +--------------+        |                            | g |
  // |        |          bitmap              +----------------------------+ i |
  // |        |          margin                    text margin              n |
  // +------------------------------------------------------------------------+
  // task bar     |                  System Systray                           |

  // TODO(levin): This is laid out by hand but it may be better to switch to
  // a GridLayout.
  int extra_glow_width = 0, extra_glow_height = 0;
  GetGlowAdditions(&extra_glow_width, &extra_glow_height);
  views::Label* drop_box_text = new views::Label(text);
  drop_box_text->SetMultiLine(true);
  drop_box_text->SetEnabled(false);
  drop_box_text->SetColor(SK_ColorWHITE);
  drop_box_text->SetBorder(
      views::Border::CreateEmptyBorder(
          kTextMargin, kTextMargin, kTextMargin, kTextMargin));
  AddChildView(drop_box_text);

  drop_box_text->SizeToFit(kMaxTextWidth);
  int text_width_required = drop_box_text->GetLocalBounds(true).width();
  int text_width = std::max<int>(systray.width(), text_width_required);

  int text_height_required = drop_box_text->GetPreferredSize().height();
  int text_height = std::max<int>(product_logo_->height(),
                                  text_height_required);

  int text_left_edge =
      extra_glow_width + kBitmapMargin + product_logo_->width();
  int text_top_edge = extra_glow_height;
  drop_box_text->SetBounds(text_left_edge, text_top_edge,
                           text_width, text_height);

  int total_width = (extra_glow_width + kBitmapMargin +
                     product_logo_->width() + drop_box_text->width());
  int total_height = extra_glow_height + drop_box_text->height();
  contents_size_.SetSize(total_width, total_height);

  // Create the only real window to enable everything to be displayed.
  container_ = new views::ContainerWin();
  container_->set_window_style(WS_POPUP);
  container_->set_window_ex_style(
      WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW);
  gfx::Rect drop_box_position(systray.right() - contents_size_.width(),
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

void BackgroundTaskDropView::GetGlowAdditions(int* x, int* y) const {
  DCHECK(x && y);
  *x = std::max<int>(
      glow_top_left_->width() - drop_box_inactive_top_left_->width(), 0);
  *y = std::max<int>(
      glow_top_left_->height() - drop_box_inactive_top_left_->height(), 0);
}

// BackgroundTaskDropView, ChromeViews::View overrides

void BackgroundTaskDropView::Paint(ChromeCanvas* canvas) {
  DCHECK(canvas);

  // Is the glow animation running?
  if (glow_animation_.get() && glow_animation_->IsAnimating()) {
    double glow_opacity = glow_animation_->GetCurrentValue();
    // Draw the glow bitmap with a varying alpha level.
    canvas->saveLayerAlpha(
        NULL,
        static_cast<int>(glow_opacity * 255),
        SkCanvas::kARGB_NoClipLayer_SaveFlag);

    // Clear the layer so we don't alpha blend with white).
    canvas->drawARGB(0, 0, 0, 0, SkPorterDuff::kSrc_Mode);

    // Draw a nine grid for the glow under the drop area.
    int widths[3] = { glow_top_left_->width(),
                      width() - glow_top_left_->width(),
                      0 };
    int heights[3] = { glow_top_left_->height(),
                       height() - glow_top_left_->height(),
                       0 };
    SkBitmap* (bitmaps[3][3]) = { { glow_top_left_, glow_top_, NULL },
                                  { glow_left_, NULL, NULL },
                                  { NULL, NULL, NULL } };
    DCHECK(glow_top_left_->height() == glow_top_->height());
    DCHECK(glow_top_left_->width() == glow_left_->width());
    DrawNineGrid(canvas, bitmaps, 0, 0, widths, heights);
    canvas->restore();
  }

  // Create the blended bitmaps.
  double hover_opacity = hover_animation_->GetCurrentValue();
  SkBitmap top_left = gfx::ImageOperations::CreateBlendedBitmap(
      *drop_box_inactive_top_left_,
      *drop_box_hover_top_left_,
      hover_opacity);
  SkBitmap top_center = gfx::ImageOperations::CreateBlendedBitmap(
      *drop_box_inactive_top_center_,
      *drop_box_hover_top_center_,
      hover_opacity);
  SkBitmap left = gfx::ImageOperations::CreateBlendedBitmap(
      *drop_box_inactive_left_,
      *drop_box_hover_left_,
      hover_opacity);
  SkBitmap center = gfx::ImageOperations::CreateBlendedBitmap(
      *drop_box_inactive_center_,
      *drop_box_hover_center_,
      hover_opacity);

  // Draw a nine grid for the drop area.
  int extra_glow_width = 0, extra_glow_height = 0;
  GetGlowAdditions(&extra_glow_width, &extra_glow_height);
  int widths[3] = { top_left.width(),
                    width() - top_left.width() - extra_glow_width,
                    0 };
  int heights[3] = { top_left.height(),
                     height() - top_left.height() - extra_glow_height,
                     0 };
  SkBitmap* (bitmaps[3][3]) = { { &top_left, &top_center, NULL },
                                { &left, &center, NULL },
                                { NULL, NULL, NULL } };
  DCHECK(top_left.height() == top_center.height());
  DCHECK(top_left.width() == left.width());
  DrawNineGrid(
      canvas, bitmaps, extra_glow_width, extra_glow_height, widths, heights);

  int logo_left_edge = extra_glow_width + kBitmapMargin;
  // Center the logo within the non-glow portion of the drop box.
  int logo_top_edge =
      (height() + extra_glow_height - product_logo_->height()) / 2;
  canvas->DrawBitmapInt(*product_logo_,
                        0,
                        0,
                        product_logo_->width(),
                        product_logo_->height(),
                        logo_left_edge,
                        logo_top_edge,
                        product_logo_->width(),
                        product_logo_->height(),
                        false);
  // Paint any contained views (like the text).
  View::Paint(canvas);
}

gfx::Size BackgroundTaskDropView::GetPreferredSize() {
  return contents_size_;
}

bool BackgroundTaskDropView::CanDrop(const OSExchangeData& data) {
  BbDragData bb_data;
  return bb_data.Read(data);
}

void BackgroundTaskDropView::OnDragEntered(
    const views::DropTargetEvent& event) {
  // Stop the glow animation when the user finds the drop target.
  glow_animation_.reset();
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
    if (background_task_manager->RegisterTask(
            source_,
            bb_data.title(),
            bb_data.url(),
            START_BACKGROUND_TASK_ON_BROWSER_LAUNCH)) {
      background_task_manager->StartTask(source_, bb_data.title());
    }
  }

  return DragDropTypes::DRAG_LINK;
}


// BackgroundTaskDropView, AnimationDelegate implementation:
// Allows the box to transition from the inactive state to the hover state.

void BackgroundTaskDropView::AnimationProgressed(const Animation* animation) {
  SchedulePaint();
}

void BackgroundTaskDropView::AnimationCanceled(const Animation* animation) {
  if (animation == glow_animation_.get()) {
    glow_animation_.reset();
  }
  SchedulePaint();
}

void BackgroundTaskDropView::AnimationEnded(const Animation* animation) {
  SchedulePaint();
}

#endif  // ENABLE_BACKGROUND_TASK
