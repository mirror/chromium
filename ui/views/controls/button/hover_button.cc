// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/views/controls/button/hover_button.h"

#include "ui/views/background.h"
#include "ui/views/border.h"
#include "ui/views/focus/focus_manager.h"
#include "ui/views/layout/grid_layout.h"
#include "ui/views/layout/layout_provider.h"
#include "ui/views/style/typography.h"

namespace views {

namespace {

constexpr float kFixed = 0.f;
constexpr float kStretchy = 1.f;

}  // namespace

HoverButton::HoverButton(ButtonListener* button_listener,
                         const base::string16& text)
    : LabelButton(button_listener, text), title_(nullptr), subtitle_(nullptr) {
  DCHECK(button_listener);
  SetFocusForPlatform();
  SetFocusPainter(nullptr);

  LayoutProvider* layout_provider = LayoutProvider::Get();
  const int horz_spacing =
      layout_provider->GetDistanceMetric(DISTANCE_BUTTON_HORIZONTAL_PADDING);
  const int vert_spacing =
      layout_provider->GetDistanceMetric(DISTANCE_UNRELATED_CONTROL_VERTICAL) /
      2;
  SetBorder(CreateEmptyBorder(vert_spacing, horz_spacing, vert_spacing,
                              horz_spacing));
}

HoverButton::HoverButton(ButtonListener* button_listener,
                         const gfx::ImageSkia& icon,
                         const base::string16& text)
    : HoverButton(button_listener, text) {
  SetImage(STATE_NORMAL, icon);
}

HoverButton::HoverButton(ButtonListener* button_listener,
                         views::View* icon_view,
                         const base::string16& title,
                         const base::string16& subtitle)
    : HoverButton(button_listener, base::string16()) {
  label()->SetHandlesTooltips(false);

  LayoutProvider* layout_provider = LayoutProvider::Get();
  const int horz_spacing =
      layout_provider->GetDistanceMetric(DISTANCE_BUTTON_HORIZONTAL_PADDING);

  const int total_height = icon_view->GetPreferredSize().height() +
                           layout_provider->GetDistanceMetric(
                               DISTANCE_CONTROL_VERTICAL_TEXT_PADDING) *
                               2;
  int remaining_vert_spacing = layout_provider->GetDistanceMetric(
      DISTANCE_CONTROL_VERTICAL_TEXT_PADDING);

  // If the padding given to the top and bottom of the HoverButton (i.e., on
  // either side of the |icon_view|) overlaps with the combined line height of
  // the |title_| and |subtitle_|, calculate the remaining padding that is
  // required to maintain a constant amount of padding above and below the icon.
  const int num_labels = subtitle.empty() ? 1 : 2;
  const int combined_line_height =
      views::style::GetLineHeight(views::style::CONTEXT_LABEL,
                                  views::style::STYLE_PRIMARY) *
      num_labels;
  if (combined_line_height > icon_view->GetPreferredSize().height()) {
    remaining_vert_spacing = (total_height - combined_line_height) / 2;
  }

  SetBorder(CreateEmptyBorder(remaining_vert_spacing, horz_spacing,
                              remaining_vert_spacing, horz_spacing));

  views::GridLayout* grid_layout = views::GridLayout::CreateAndInstall(this);
  const int icon_label_spacing =
      layout_provider->GetDistanceMetric(DISTANCE_RELATED_LABEL_HORIZONTAL);

  constexpr int kColumnSetId = 0;
  views::ColumnSet* columns = grid_layout->AddColumnSet(kColumnSetId);
  columns->AddColumn(views::GridLayout::CENTER, views::GridLayout::CENTER,
                     kFixed, views::GridLayout::USE_PREF, 0, 0);
  columns->AddPaddingColumn(kFixed, icon_label_spacing);
  columns->AddColumn(views::GridLayout::LEADING, views::GridLayout::FILL,
                     kStretchy, views::GridLayout::USE_PREF, 0, 0);

  grid_layout->StartRow(0, 0,
                        icon_view->GetPreferredSize().height() / num_labels);
  grid_layout->AddView(icon_view, 1, num_labels);

  title_ = new views::Label(title);
  grid_layout->AddView(title_);

  if (!subtitle.empty()) {
    grid_layout->StartRow(0, 0);
    subtitle_ = new views::Label(subtitle);
    grid_layout->SkipColumns(1);
    grid_layout->AddView(subtitle_);
  }

  SetMinSize(
      gfx::Size(grid_layout->GetPreferredSize(this).width(), total_height));
}

void HoverButton::SetSubtitleElideBehavior(gfx::ElideBehavior elide_behavior) {
  DCHECK(subtitle_);
  if (!subtitle_->text().empty())
    subtitle_->SetElideBehavior(elide_behavior);
}

void HoverButton::OnFocus() {
  LabelButton::OnFocus();
  UpdateColors();
}

void HoverButton::OnBlur() {
  LabelButton::OnBlur();
  UpdateColors();
}

void HoverButton::OnNativeThemeChanged(const ui::NativeTheme* theme) {
  LabelButton::OnNativeThemeChanged(theme);
  UpdateColors();
}

void HoverButton::StateChanged(ButtonState old_state) {
  LabelButton::StateChanged(old_state);

  // As in a menu, focus follows the mouse (including blurring when the mouse
  // leaves the button). If we don't do this, the focused view and the hovered
  // view might both have the selection highlight.
  if (state() == STATE_HOVERED || state() == STATE_PRESSED)
    RequestFocus();
  else if (state() == STATE_NORMAL && HasFocus())
    GetFocusManager()->SetFocusedView(nullptr);

  UpdateColors();
}

void HoverButton::UpdateColors() {
  // TODO(tapted): This should use style::GetColor() to obtain grey text where
  // it's needed. But note |subtitle_| is often used to draw an email, which
  // might not be grey in Harmony.
  bool is_selected = HasFocus();

  SetBackground(
      is_selected
          ? CreateSolidBackground(GetNativeTheme()->GetSystemColor(
                ui::NativeTheme::kColorId_FocusedMenuItemBackgroundColor))
          : nullptr);

  SkColor text_color = GetNativeTheme()->GetSystemColor(
      is_selected ? ui::NativeTheme::kColorId_SelectedMenuItemForegroundColor
                  : ui::NativeTheme::kColorId_LabelEnabledColor);
  SetEnabledTextColors(text_color);
  if (title_)
    title_->SetEnabledColor(text_color);

  if (subtitle_) {
    subtitle_->SetEnabledColor(GetNativeTheme()->GetSystemColor(
        is_selected ? ui::NativeTheme::kColorId_DisabledMenuItemForegroundColor
                    : ui::NativeTheme::kColorId_LabelDisabledColor));
  }
}

}  // namespace views
