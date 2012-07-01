// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/tray/tray_bubble_view.h"

#include "ash/shell.h"
#include "ash/shell_window_ids.h"
#include "ash/system/tray/tray_constants.h"
#include "ash/wm/shelf_layout_manager.h"
#include "grit/ash_strings.h"
#include "third_party/skia/include/core/SkCanvas.h"
#include "third_party/skia/include/core/SkColor.h"
#include "third_party/skia/include/core/SkPaint.h"
#include "third_party/skia/include/core/SkPath.h"
#include "third_party/skia/include/effects/SkBlurImageFilter.h"
#include "ui/aura/event.h"
#include "ui/aura/window.h"
#include "ui/base/accessibility/accessible_view_state.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/screen.h"
#include "ui/views/bubble/bubble_frame_view.h"
#include "ui/views/layout/box_layout.h"

namespace ash {

namespace {

const int kShadowThickness = 4;
const int kBottomLineHeight = 1;
const int kSystemTrayBubbleHorizontalInset = 1;
const int kSystemTrayBubbleVerticalInset = 1;

const int kArrowHeight = 10;
const int kArrowWidth = 20;

// Inset the arrow a bit from the edge.
const int kArrowMinOffset = kArrowWidth / 2 + 4;

const SkColor kShadowColor = SkColorSetARGB(0xff, 0, 0, 0);

void DrawBlurredShadowAroundView(gfx::Canvas* canvas,
                                 int top,
                                 int bottom,
                                 int width,
                                 const gfx::Insets& inset) {
  SkPath path;
  path.incReserve(4);
  path.moveTo(SkIntToScalar(inset.left() + kShadowThickness),
              SkIntToScalar(top + kShadowThickness + 1));
  path.lineTo(SkIntToScalar(inset.left() + kShadowThickness),
              SkIntToScalar(bottom));
  path.lineTo(SkIntToScalar(width),
              SkIntToScalar(bottom));
  path.lineTo(SkIntToScalar(width),
              SkIntToScalar(top + kShadowThickness + 1));

  SkPaint paint;
  paint.setColor(kShadowColor);
  paint.setStyle(SkPaint::kStroke_Style);
  paint.setXfermodeMode(SkXfermode::kSrcOver_Mode);
  paint.setStrokeWidth(SkIntToScalar(3));
  paint.setImageFilter(new SkBlurImageFilter(
      SkIntToScalar(3), SkIntToScalar(3)))->unref();
  canvas->sk_canvas()->drawPath(path, paint);
}

class TrayBubbleBorder : public views::BubbleBorder {
 public:
  TrayBubbleBorder(views::View* owner,
                   views::View* anchor,
                   views::BubbleBorder::ArrowLocation arrow_location,
                   int arrow_offset)
      : views::BubbleBorder(arrow_location,
                            views::BubbleBorder::NO_SHADOW),
        owner_(owner),
        anchor_(anchor),
        tray_arrow_offset_(arrow_offset) {
    set_alignment(views::BubbleBorder::ALIGN_EDGE_TO_ANCHOR_EDGE);
  }

  virtual ~TrayBubbleBorder() {}

 private:
  views::Background* FindAppropriateBackground(views::View* view,
                                               const gfx::Point& point) const {
    views::Background* background = NULL;
    views::View* target = view->GetEventHandlerForPoint(point);
    for (; target && !background; target = target->parent())
      background = target->background();
    return background;
  }

  // Overridden from views::BubbleBorder.
  // Override views::BubbleBorder to set the bubble on top of the anchor when
  // it has no arrow.
  virtual gfx::Rect GetBounds(const gfx::Rect& position_relative_to,
                              const gfx::Size& contents_size) const OVERRIDE {
    if (arrow_location() != NONE) {
      return views::BubbleBorder::GetBounds(position_relative_to,
                                            contents_size);
    }

    gfx::Size border_size(contents_size);
    gfx::Insets insets;
    GetInsets(&insets);
    border_size.Enlarge(insets.width(), insets.height());

    const int kArrowOverlap = 3;
    int x = position_relative_to.x() +
        position_relative_to.width() / 2 - border_size.width() / 2;
    // Position the bubble on top of the anchor.
    int y = position_relative_to.y() +
        kArrowOverlap - border_size.height();
    return gfx::Rect(x, y, border_size.width(), border_size.height());
  }

  // Overridden from views::Border.
  virtual void Paint(const views::View& view,
                     gfx::Canvas* canvas) const OVERRIDE {
    gfx::Insets inset;
    GetInsets(&inset);
    DrawBlurredShadowAroundView(
        canvas, 0, owner_->height(), owner_->width(), inset);

    // Draw the bottom line.
    int y = owner_->height() + inset.top();
    canvas->FillRect(gfx::Rect(inset.left(), y, owner_->width(),
                               kBottomLineHeight), kBorderDarkColor);

    if (!Shell::GetInstance()->shelf()->IsVisible() ||
        arrow_location() == views::BubbleBorder::NONE)
      return;

    gfx::Point arrow_reference;

    // Draw the arrow after drawing child borders, so that the arrow can cover
    // the its overlap section with child border.
    SkPath path;
    path.incReserve(4);
    if (arrow_location() == views::BubbleBorder::BOTTOM_RIGHT ||
        arrow_location() == views::BubbleBorder::BOTTOM_LEFT) {
      // Do not let the arrow too close to the edge of the bubble and
      // and the edge of the anchor.
      int tip_x = base::i18n::IsRTL() ?
          std::min(std::max(tray_arrow_offset_, kArrowMinOffset),
                   std::min(owner_->width(), anchor_->width())
                       - kArrowMinOffset) :
          std::min(std::max(owner_->width() - tray_arrow_offset_,
                            owner_->width() -
                                std::min(owner_->width(), anchor_->width()) +
                                    kArrowMinOffset),
                   owner_->width() - kArrowMinOffset);
      int left_base_x = tip_x - kArrowWidth / 2;
      int left_base_y = y;
      int tip_y = left_base_y + kArrowHeight;
      path.moveTo(SkIntToScalar(left_base_x), SkIntToScalar(left_base_y));
      path.lineTo(SkIntToScalar(tip_x), SkIntToScalar(tip_y));
      path.lineTo(SkIntToScalar(left_base_x + kArrowWidth),
                  SkIntToScalar(left_base_y));
      arrow_reference.SetPoint(tip_x, left_base_y - kArrowHeight);
    } else {
      int tip_y = y - tray_arrow_offset_;
      tip_y = std::min(std::max(kArrowMinOffset, tip_y),
                       owner_->height() - kArrowMinOffset);
      int top_base_y = tip_y - kArrowWidth / 2;
      int top_base_x, tip_x;
      if (arrow_location() == views::BubbleBorder::LEFT_BOTTOM) {
        top_base_x = inset.left() + kSystemTrayBubbleHorizontalInset;
        tip_x = top_base_x - kArrowHeight;
        arrow_reference.SetPoint(top_base_x + kArrowHeight, tip_y);
      } else {
        DCHECK(arrow_location() == views::BubbleBorder::RIGHT_BOTTOM);
        top_base_x = inset.left() + owner_->width() -
            kSystemTrayBubbleHorizontalInset;
        tip_x = top_base_x + kArrowHeight;
        arrow_reference.SetPoint(top_base_x - kArrowHeight, tip_y);
      }
      path.moveTo(SkIntToScalar(top_base_x), SkIntToScalar(top_base_y));
      path.lineTo(SkIntToScalar(tip_x), SkIntToScalar(tip_y));
      path.lineTo(SkIntToScalar(top_base_x),
                  SkIntToScalar(top_base_y + kArrowWidth));
    }

    views::Background* background = FindAppropriateBackground(owner_,
                                                              arrow_reference);

    SkPaint paint;
    paint.setStyle(SkPaint::kFill_Style);
    paint.setColor(background ? background->get_color() : kBackgroundColor);
    canvas->DrawPath(path, paint);

    // Now draw the arrow border.
    paint.setStyle(SkPaint::kStroke_Style);
    paint.setColor(kBorderDarkColor);
    canvas->DrawPath(path, paint);

  }

  views::View* owner_;
  views::View* anchor_;
  const int tray_arrow_offset_;

  DISALLOW_COPY_AND_ASSIGN(TrayBubbleBorder);
};

}  // namespace

namespace internal {

TrayBubbleView::TrayBubbleView(
    views::View* anchor,
    views::BubbleBorder::ArrowLocation arrow_location,
    Host* host,
    bool can_activate,
    int bubble_width)
    : views::BubbleDelegateView(anchor, arrow_location),
      host_(host),
      can_activate_(can_activate),
      max_height_(0),
      bubble_width_(bubble_width) {
  set_margin(0);
  set_parent_window(Shell::GetContainer(
      anchor->GetWidget()->GetNativeWindow()->GetRootWindow(),
      internal::kShellWindowId_SettingBubbleContainer));
  set_notify_enter_exit_on_child(true);
  SetPaintToLayer(true);
  SetFillsBoundsOpaquely(true);
}

TrayBubbleView::~TrayBubbleView() {
  // Inform host items (models) that their views are being destroyed.
  if (host_)
    host_->BubbleViewDestroyed();
}

void TrayBubbleView::SetBubbleBorder(int arrow_offset) {
  DCHECK(GetWidget());
  TrayBubbleBorder* bubble_border = new TrayBubbleBorder(
      this, anchor_view(), arrow_location(), arrow_offset);
  GetBubbleFrameView()->SetBubbleBorder(bubble_border);
  // Recalculate size with new border.
  SizeToContents();
}

void TrayBubbleView::UpdateAnchor() {
  SizeToContents();
  GetWidget()->GetRootView()->SchedulePaint();
}

void TrayBubbleView::SetMaxHeight(int height) {
  max_height_ = height;
  if (GetWidget())
    SizeToContents();
}

void TrayBubbleView::Init() {
  views::BoxLayout* layout =
      new views::BoxLayout(views::BoxLayout::kVertical, 0, 0, 0);
  layout->set_spread_blank_space(true);
  SetLayoutManager(layout);
  set_background(NULL);
}

gfx::Rect TrayBubbleView::GetAnchorRect() {
  gfx::Rect rect;
  if (host_)
    rect = host_->GetAnchorRect();
  // TODO(jennyz): May need to add left/right alignment in the following code.
  if (rect.IsEmpty()) {
    rect = gfx::Screen::GetPrimaryDisplay().bounds();
    rect = gfx::Rect(
        base::i18n::IsRTL() ? kPaddingFromRightEdgeOfScreenBottomAlignment :
        rect.width() - kPaddingFromRightEdgeOfScreenBottomAlignment,
        rect.height() - kPaddingFromBottomOfScreenBottomAlignment,
        0, 0);
  }
  return rect;
}

gfx::Rect TrayBubbleView::GetBubbleBounds() {
  // Same as BubbleDelegateView implementation, but don't try mirroring.
  return GetBubbleFrameView()->GetUpdatedWindowBounds(
      GetAnchorRect(), GetPreferredSize(), false /*try_mirroring_arrow*/);
}

bool TrayBubbleView::CanActivate() const {
  return can_activate_;
}

gfx::Size TrayBubbleView::GetPreferredSize() {
  gfx::Size size = views::BubbleDelegateView::GetPreferredSize();
  int height = size.height();
  if (max_height_ != 0 && height > max_height_)
    height = max_height_;
  return gfx::Size(bubble_width_, height);
}

void TrayBubbleView::OnMouseEntered(const views::MouseEvent& event) {
  if (host_)
    host_->OnMouseEnteredView();
}

void TrayBubbleView::OnMouseExited(const views::MouseEvent& event) {
  if (host_)
    host_->OnMouseExitedView();
}

void TrayBubbleView::GetAccessibleState(ui::AccessibleViewState* state) {
  if (can_activate_) {
    state->role = ui::AccessibilityTypes::ROLE_WINDOW;
    state->name = l10n_util::GetStringUTF16(
        IDS_ASH_STATUS_TRAY_ACCESSIBLE_NAME);
  }
}

void TrayBubbleView::ChildPreferredSizeChanged(View* child) {
  SizeToContents();
}

void TrayBubbleView::ViewHierarchyChanged(bool is_add,
                                          views::View* parent,
                                          views::View* child) {
  if (is_add && child == this) {
    parent->SetPaintToLayer(true);
    parent->SetFillsBoundsOpaquely(true);
    parent->layer()->SetMasksToBounds(true);
  }
}

TrayBubbleView::Host::Host()
    : widget_(NULL),
      tray_view_(NULL) {
  Shell::GetInstance()->AddEnvEventFilter(this);
}

TrayBubbleView::Host::~Host() {
  Shell::GetInstance()->RemoveEnvEventFilter(this);
}

void TrayBubbleView::Host::InitializeHost(views::Widget* widget,
                                          views::View* tray_view) {
  widget_ = widget;
  tray_view_ = tray_view;
}

bool TrayBubbleView::Host::PreHandleKeyEvent(aura::Window* target,
                                             aura::KeyEvent* event) {
  return false;
}

bool TrayBubbleView::Host::PreHandleMouseEvent(aura::Window* target,
                                               aura::MouseEvent* event) {
  if (event->type() == ui::ET_MOUSE_PRESSED)
    ProcessLocatedEvent(*event);
  return false;
}

ui::TouchStatus TrayBubbleView::Host::PreHandleTouchEvent(
    aura::Window* target,
    aura::TouchEvent* event) {
  if (event->type() == ui::ET_TOUCH_PRESSED)
    ProcessLocatedEvent(*event);
  return ui::TOUCH_STATUS_UNKNOWN;
}

ui::GestureStatus TrayBubbleView::Host::PreHandleGestureEvent(
    aura::Window* target,
    aura::GestureEvent* event) {
  return ui::GESTURE_STATUS_UNKNOWN;
}

void TrayBubbleView::Host::ProcessLocatedEvent(
    const aura::LocatedEvent& event) {
  if (!widget_)
    return;
  gfx::Rect bounds = widget_->GetNativeWindow()->GetBoundsInRootWindow();
  if (bounds.Contains(event.root_location()))
    return;
  if (tray_view_) {
    // If the user clicks on the parent tray, don't process the event here,
    // let the tray logic handle the event and determine show/hide behavior.
    bounds = tray_view_->ConvertRectToWidget(tray_view_->GetLocalBounds());
    if (bounds.Contains(event.location()))
      return;
  }
  // Handle clicking outside the bubble and tray. We don't block the event, so
  // it will also be handled by whatever widget was clicked on.
  OnClickedOutsideView();
}


}  // namespace internal
}  // namespace ash
