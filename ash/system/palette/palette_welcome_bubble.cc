// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/palette/palette_welcome_bubble.h"

#include "ash/public/cpp/ash_pref_names.h"
#include "ash/public/cpp/shell_window_ids.h"
#include "ash/resources/vector_icons/vector_icons.h"
#include "ash/session/session_controller.h"
#include "ash/shell.h"
#include "ash/shell_port.h"
#include "ash/strings/grit/ash_strings.h"
#include "ash/system/palette/palette_tray.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "ui/aura/window.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/gfx/font.h"
#include "ui/gfx/paint_vector_icon.h"
#include "ui/views/bubble/bubble_border.h"
#include "ui/views/bubble/bubble_dialog_delegate.h"
#include "ui/views/bubble/bubble_frame_view.h"
#include "ui/views/controls/button/image_button.h"
#include "ui/views/controls/label.h"
#include "ui/views/layout/box_layout.h"

namespace ash {

namespace {

// Padding of the bubble.
constexpr int kBubbleTopPaddingDp = 14;
constexpr int kBubbleLeftBottomPaddingDp = 16;

constexpr int kBubbleSpaceBetweenChildrenDp = 8;

// Colors of the two text in the bubble.
constexpr SkColor kBubbleTitleTextColor = SK_ColorBLACK;
constexpr SkColor kBubbleContentTextColor = SkColorSetA(SK_ColorBLACK, 138);

// Font sizes of the two text in the bubble.
constexpr int kBubbleTitleTextFontSize = 16;
constexpr int kBubbleContentTextFontSize = 13;

// Height and width of the preferred bubble size.
constexpr int kBubblePreferredHeightDp = 96;
constexpr int kBubblePreferredWidthDp = 380;

// The preferred size of the close button.
constexpr gfx::Size kCloseButtonPreferredSizeDp = gfx::Size(16, 16);

// The location of the arrow on the bubble, expressed as a ratio of the bubble's
// width.
constexpr float kArrowLocationRatio = 0.75f;

}  // namespace

// Custom border for the bubble, as this arrow on this bubble points to its
// anchor at 3/4 of its size. BubbleBorder only provides a choice of pointing at
// the beginning, center or end.
class PaletteWelcomeBubble::CustomBubbleBorder : public views::BubbleBorder {
 public:
  CustomBubbleBorder()
      : views::BubbleBorder(views::BubbleBorder::BOTTOM_CENTER,
                            views::BubbleBorder::NO_SHADOW,
                            SK_ColorWHITE) {}
  ~CustomBubbleBorder() override = default;

  // views::BubbleBorder:
  gfx::Rect GetBounds(const gfx::Rect& anchor_rect,
                      const gfx::Size& contents_size) const override {
    gfx::Rect bounds(contents_size);
    bounds.set_x(anchor_rect.CenterPoint().x() -
                 kArrowLocationRatio * contents_size.width());
    bounds.set_y(anchor_rect.y() - contents_size.height());
    return bounds;
  }

  gfx::Rect GetArrowRect(const gfx::Rect& bounds) const override {
    gfx::Rect rect = BubbleBorder::GetArrowRect(bounds);
    rect.set_x(kArrowLocationRatio * bounds.width() -
               GetArrowImage()->size().width() / 2);
    return rect;
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(CustomBubbleBorder);
};

// View which contains two text (one title) and a close button for closing the
// bubble. Controlled by PaletteWelcomeBubble and anchored to a PaletteTray.
class PaletteWelcomeBubble::WelcomeBubbleView
    : public views::BubbleDialogDelegateView,
      public views::ButtonListener {
 public:
  explicit WelcomeBubbleView(views::View* anchor)
      : views::BubbleDialogDelegateView(anchor,
                                        views::BubbleBorder::BOTTOM_CENTER) {
    set_close_on_deactivate(true);
    set_can_activate(false);
    set_accept_events(true);
    set_parent_window(
        anchor_widget()->GetNativeWindow()->GetRootWindow()->GetChildById(
            kShellWindowId_SettingBubbleContainer));
    set_margins(gfx::Insets());
    auto* layout = new views::BoxLayout(
        views::BoxLayout::kVertical,
        gfx::Insets(kBubbleTopPaddingDp, kBubbleLeftBottomPaddingDp,
                    kBubbleLeftBottomPaddingDp, kBubbleLeftBottomPaddingDp),
        kBubbleSpaceBetweenChildrenDp);
    SetLayoutManager(layout);

    // Add the header which contains the title and close button.
    auto* header = new views::View();
    auto* header_layout = new views::BoxLayout(views::BoxLayout::kHorizontal);
    header->SetLayoutManager(header_layout);
    AddChildView(header);

    const int default_font_size =
        views::Label::GetDefaultFontList().GetFontSize();
    // Add the title.
    auto* title = new views::Label(
        l10n_util::GetStringUTF16(IDS_ASH_STYLUS_WARM_WELCOME_BUBBLE_TITLE));
    title->SetEnabledColor(kBubbleTitleTextColor);
    title->SetFontList(views::Label::GetDefaultFontList().DeriveWithSizeDelta(
        kBubbleTitleTextFontSize - default_font_size));
    title->SetHorizontalAlignment(gfx::ALIGN_LEFT);
    header->AddChildView(title);
    header_layout->SetFlexForView(title, 1);

    // Add a button to close the bubble.
    close_button_ = new views::ImageButton(this);
    close_button_->SetImage(
        views::Button::STATE_NORMAL,
        gfx::CreateVectorIcon(kWindowControlCloseIcon, SK_ColorBLACK));
    close_button_->SetPreferredSize(gfx::Size(kCloseButtonPreferredSizeDp));
    header->AddChildView(close_button_);

    // Add the content.
    auto* content = new views::Label(l10n_util::GetStringUTF16(
        IDS_ASH_STYLUS_WARM_WELCOME_BUBBLE_DESCRIPTION));
    content->SetEnabledColor(kBubbleContentTextColor);
    content->SetFontList(views::Label::GetDefaultFontList().DeriveWithSizeDelta(
        kBubbleContentTextFontSize - default_font_size));
    content->SetHorizontalAlignment(gfx::ALIGN_LEFT);
    content->SetMultiLine(true);
    AddChildView(content);

    views::BubbleDialogDelegateView::CreateBubble(this);
    GetBubbleFrameView()->SetBubbleBorder(
        std::make_unique<CustomBubbleBorder>());
    SizeToContents();
  }

  ~WelcomeBubbleView() override = default;

  views::ImageButton* close_button() { return close_button_; }

  // ui::BubbleDialogDelegateView:
  int GetDialogButtons() const override { return ui::DIALOG_BUTTON_NONE; }

  // views::ButtonListener:
  void ButtonPressed(views::Button* sender, const ui::Event& event) override {
    return;
    if (sender == close_button_)
      GetWidget()->Close();
  }

  // views::View:
  gfx::Size CalculatePreferredSize() const override {
    const int preferred_height =
        GetLayoutManager()->GetPreferredSize(this).height() +
        kBubbleLeftBottomPaddingDp;
    return gfx::Size(kBubblePreferredWidthDp,
                     std::max(preferred_height, kBubblePreferredHeightDp));
  }

  void Layout() override {
    views::View::Layout();
    SizeToPreferredSize();
  }

 private:
  views::ImageButton* close_button_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(WelcomeBubbleView);
};

PaletteWelcomeBubble::PaletteWelcomeBubble(PaletteTray* tray) : tray_(tray) {
  Shell::Get()->session_controller()->AddObserver(this);
}

PaletteWelcomeBubble::~PaletteWelcomeBubble() {
  if (bubble_view_) {
    bubble_view_->GetWidget()->RemoveObserver(this);
    ShellPort::Get()->RemovePointerWatcher(this);
  }
  Shell::Get()->session_controller()->RemoveObserver(this);
}

// static
void PaletteWelcomeBubble::RegisterProfilePrefs(PrefRegistrySimple* registry) {
  registry->RegisterBooleanPref(prefs::kShownPaletteWelcomeBubble, false);
}

void PaletteWelcomeBubble::OnWidgetClosing(views::Widget* widget) {
  widget->RemoveObserver(this);
  bubble_view_ = nullptr;
  ShellPort::Get()->RemovePointerWatcher(this);
}

void PaletteWelcomeBubble::OnActiveUserPrefServiceChanged(
    PrefService* pref_service) {
  active_user_pref_service_ = pref_service;
}

void PaletteWelcomeBubble::ShowIfNeeded(bool shown_by_stylus) {
  DCHECK(active_user_pref_service_);
  if (!active_user_pref_service_->GetBoolean(
          prefs::kShownPaletteWelcomeBubble) &&
      !bubble_shown()) {
    Show(shown_by_stylus);
  }
}

void PaletteWelcomeBubble::MarkAsShown() {
  DCHECK(active_user_pref_service_);
  active_user_pref_service_->SetBoolean(prefs::kShownPaletteWelcomeBubble,
                                        true);
}

views::ImageButton* PaletteWelcomeBubble::GetCloseButtonForTest() {
  if (bubble_view_)
    return bubble_view_->close_button();

  return nullptr;
}

base::Optional<gfx::Rect> PaletteWelcomeBubble::GetBubbleBoundsForTest() {
  if (bubble_view_)
    return base::make_optional(bubble_view_->GetBoundsInScreen());

  return base::nullopt;
}

void PaletteWelcomeBubble::Show(bool shown_by_stylus) {
  if (!bubble_view_) {
    DCHECK(tray_);
    bubble_view_ = new WelcomeBubbleView(tray_);
  }
  active_user_pref_service_->SetBoolean(prefs::kShownPaletteWelcomeBubble,
                                        true);
  bubble_view_->GetWidget()->Show();
  bubble_view_->GetWidget()->AddObserver(this);
  ShellPort::Get()->AddPointerWatcher(this,
                                      views::PointerWatcherEventTypes::BASIC);

  // If the bubble is shown after the device first reads a stylus, ignore the
  // first up event so the event responsible for showing the bubble does not
  // also cause the bubble to close.
  if (shown_by_stylus)
    ignore_stylus_event_ = true;
}

void PaletteWelcomeBubble::Hide() {
  if (bubble_view_)
    bubble_view_->GetWidget()->Close();
}

void PaletteWelcomeBubble::OnPointerEventObserved(
    const ui::PointerEvent& event,
    const gfx::Point& location_in_screen,
    gfx::NativeView target) {
  if (!bubble_view_)
    return;

  // For devices with external stylus, the bubble is activated after it receives
  // a pen pointer event. However, on attaching |this| as a pointer watcher, it
  // receives the same set of events which activated the bubble in the first
  // place (ET_POINTER_DOWN, ET_POINTER_UP, ET_POINTER_CAPTURE_CHANGED). By only
  // looking at ET_POINTER_UP events and skipping the first up event if the
  // bubble is shown because of a stylus event, we ensure the same event that
  // opened the bubble does not close it as well.
  if (event.type() != ui::ET_POINTER_UP)
    return;

  if (ignore_stylus_event_) {
    ignore_stylus_event_ = false;
    return;
  }

  if (!bubble_view_->GetBoundsInScreen().Contains(location_in_screen))
    bubble_view_->GetWidget()->Close();
}

}  // namespace ash
