// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/hover_button.h"

#include "chrome/browser/ui/views/harmony/chrome_layout_provider.h"
#include "chrome/browser/ui/views/harmony/chrome_typography.h"
#include "ui/base/material_design/material_design_controller.h"
#include "ui/views/animation/ink_drop_highlight.h"
#include "ui/views/animation/ink_drop_ripple.h"
#include "ui/views/background.h"
#include "ui/views/border.h"
#include "ui/views/controls/styled_label.h"
#include "ui/views/focus/focus_manager.h"
#include "ui/views/layout/grid_layout.h"

namespace {

std::unique_ptr<views::Border> CreateBorderWithVerticalSpacing(
    int vert_spacing) {
  const int horz_spacing = ChromeLayoutProvider::Get()->GetDistanceMetric(
      views::DISTANCE_BUTTON_HORIZONTAL_PADDING);
  return views::CreateEmptyBorder(vert_spacing, horz_spacing, vert_spacing,
                                  horz_spacing);
}

}  // namespace

HoverButton::HoverButton(views::ButtonListener* button_listener,
                         const base::string16& text)
    : views::LabelButton(button_listener, text),
      title_(nullptr),
      subtitle_(nullptr) {
  DCHECK(button_listener);
  SetFocusForPlatform();
  SetFocusPainter(nullptr);
  label()->SetHandlesTooltips(false);

  const int vert_spacing = ChromeLayoutProvider::Get()->GetDistanceMetric(
      DISTANCE_CONTROL_LIST_VERTICAL);
  SetBorder(CreateBorderWithVerticalSpacing(vert_spacing));

  // Turn on highlighting when the button is hovered.
  SetInkDropMode(views::InkDropHostView::InkDropMode::ON);
}

HoverButton::HoverButton(views::ButtonListener* button_listener,
                         const gfx::ImageSkia& icon,
                         const base::string16& text)
    : HoverButton(button_listener, text) {
  SetImage(STATE_NORMAL, icon);
}

HoverButton::HoverButton(views::ButtonListener* button_listener,
                         std::unique_ptr<views::View> icon_view,
                         const base::string16& title,
                         const base::string16& subtitle)
    : HoverButton(button_listener, base::string16()) {
  ChromeLayoutProvider* layout_provider = ChromeLayoutProvider::Get();

  // The vertical spacing above and below the icon. If the icon is small, use
  // more vertical spacing.
  constexpr int kLargeIconHeight = 20;
  const int icon_height = icon_view->GetPreferredSize().height();
  int remaining_vert_spacing =
      icon_height <= kLargeIconHeight
          ? layout_provider->GetDistanceMetric(DISTANCE_CONTROL_LIST_VERTICAL)
          : layout_provider->GetDistanceMetric(
                views::DISTANCE_CONTROL_VERTICAL_TEXT_PADDING);
  const int total_height = icon_height + remaining_vert_spacing * 2;

  // If the padding given to the top and bottom of the HoverButton (i.e., on
  // either side of the |icon_view|) overlaps with the combined line height of
  // the |title_| and |subtitle_|, calculate the remaining padding that is
  // required to maintain a constant amount of padding above and below the icon.
  const int num_labels = subtitle.empty() ? 1 : 2;
  const int combined_line_height =
      views::style::GetLineHeight(views::style::CONTEXT_LABEL,
                                  views::style::STYLE_PRIMARY) *
      num_labels;
  if (combined_line_height > icon_view->GetPreferredSize().height())
    remaining_vert_spacing = (total_height - combined_line_height) / 2;

  SetBorder(CreateBorderWithVerticalSpacing(remaining_vert_spacing));

  views::GridLayout* grid_layout = views::GridLayout::CreateAndInstall(this);
  const int icon_label_spacing = layout_provider->GetDistanceMetric(
      views::DISTANCE_RELATED_LABEL_HORIZONTAL);

  constexpr float kFixed = 0.f;
  constexpr float kStretchy = 1.f;
  constexpr int kColumnSetId = 0;
  views::ColumnSet* columns = grid_layout->AddColumnSet(kColumnSetId);
  columns->AddColumn(views::GridLayout::CENTER, views::GridLayout::CENTER,
                     kFixed, views::GridLayout::USE_PREF, 0, 0);
  columns->AddPaddingColumn(kFixed, icon_label_spacing);
  columns->AddColumn(views::GridLayout::LEADING, views::GridLayout::FILL,
                     kStretchy, views::GridLayout::USE_PREF, 0, 0);

  // Make sure hovering over the icon also hovers the |HoverButton|.
  icon_view->set_can_process_events_within_subtree(false);
  // Don't cover |icon_view| when the ink drops are being painted. |LabelButton|
  // already does this with its own image.
  icon_view->SetPaintToLayer();
  icon_view->layer()->SetFillsBoundsOpaquely(false);
  // Split the two rows evenly between the total height minus the padding.
  const int row_height =
      (total_height - remaining_vert_spacing * 2) / num_labels;
  grid_layout->StartRow(0, 0, row_height);
  grid_layout->AddView(icon_view.release(), 1, num_labels);

  title_ = new views::StyledLabel(title, nullptr);
  // Hover the whole button when hovering |title_|. This is OK because |title_|
  // will never have a link in it.
  title_->set_can_process_events_within_subtree(false);
  grid_layout->AddView(title_);
  title_->SizeToFit(0);

  if (!subtitle.empty()) {
    grid_layout->StartRow(0, 0, row_height);
    subtitle_ = new views::Label(subtitle, views::style::CONTEXT_BUTTON,
                                 STYLE_SECONDARY);
    subtitle_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
    subtitle_->SetAutoColorReadabilityEnabled(false);
    grid_layout->SkipColumns(1);
    grid_layout->AddView(subtitle_);
  }

  // Since this constructor doesn't use the |LabelButton|'s image / label Views,
  // make sure the minimum size is correct according to the |grid_layout|.
  SetMinSize(gfx::Size(grid_layout->GetPreferredSize(this)));
}

HoverButton::~HoverButton() {}

void HoverButton::SetSubtitleElideBehavior(gfx::ElideBehavior elide_behavior) {
  DCHECK(subtitle_);
  if (!subtitle_->text().empty())
    subtitle_->SetElideBehavior(elide_behavior);
}

void HoverButton::SetTitleText(const base::string16& title_text) {
  DCHECK(title_);
  title_->SetText(title_text);
  title_->SizeToFit(0);
}

void HoverButton::ApplyStyleRangeToTitle(const gfx::Range& range) {
  DCHECK(title_);
  views::StyledLabel::RangeStyleInfo style_info;
  style_info.text_style = views::style::STYLE_DISABLED;
  title_->AddStyleRange(range, style_info);
  title_->SizeToFit(0);
}

bool HoverButton::ShouldUseFloodFillInkDrop() const {
  return views::LabelButton::ShouldUseFloodFillInkDrop() || title_ != nullptr;
}

SkColor HoverButton::GetInkDropBaseColor() const {
  return views::style::GetColor(views::style::CONTEXT_BUTTON,
                                views::style::STYLE_PRIMARY, GetNativeTheme());
}

std::unique_ptr<views::InkDropRipple> HoverButton::CreateInkDropRipple() const {
  if (ui::MaterialDesignController::IsSecondaryUiMaterial())
    return LabelButton::CreateInkDropRipple();
  // Don't show the ripple on non-MD.
  return nullptr;
}

std::unique_ptr<views::InkDropHighlight> HoverButton::CreateInkDropHighlight()
    const {
  // HoverButtons are supposed to encompass the full width of their parent, so
  // remove the rounded corners.
  std::unique_ptr<views::InkDropHighlight> highlight(
      new views::InkDropHighlight(
          size(), 0,
          gfx::RectF(GetMirroredRect(GetContentsBounds())).CenterPoint(),
          GetInkDropBaseColor()));
  highlight->set_explode_size(gfx::SizeF(CalculateLargeInkDropSize(size())));
  return highlight;
}
