// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/message_center/message_center_button_bar.h"

#include "ash/message_center/message_center_view.h"
#include "ash/resources/vector_icons/vector_icons.h"
#include "ash/strings/grit/ash_strings.h"
#include "base/macros.h"
#include "build/build_config.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/models/simple_menu_model.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/text_constants.h"
#include "ui/message_center/message_center.h"
#include "ui/message_center/message_center_tray.h"
#include "ui/message_center/notifier_settings.h"
#include "ui/message_center/public/cpp/message_center_constants.h"
#include "ui/resources/grit/ui_resources.h"
#include "ui/views/border.h"
#include "ui/views/controls/button/button.h"
#include "ui/views/controls/button/image_button.h"
#include "ui/views/controls/button/label_button.h"
#include "ui/views/controls/button/menu_button.h"
#include "ui/views/controls/button/menu_button_listener.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/menu/menu_runner.h"
#include "ui/views/controls/separator.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/layout/grid_layout.h"
#include "ui/views/painter.h"

using message_center::MessageCenter;
using message_center::NotifierSettingsProvider;
using views::Button;

namespace ash {

namespace {
constexpr int kButtonSize = 20;
constexpr gfx::Insets kButtonPadding(14);
constexpr SkColor kActiveButtonColor = SkColorSetARGB(0xFF, 0x5A, 0x5A, 0x5A);
constexpr SkColor kInactiveButtonColor = SkColorSetARGB(0x8A, 0x5A, 0x5A, 0x5A);
constexpr SkColor kTextColor = SkColorSetARGB(0xFF, 0x0, 0x0, 0x0);
constexpr SkColor kButtonSeparatorColor = SkColorSetARGB(0x1F, 0x0, 0x0, 0x0);
constexpr int kTextFontSize = 14;

void SetDefaultButtonStyles(views::Button* button) {
  button->SetBorder(views::CreateEmptyBorder(kButtonPadding));
  button->SetFocusForPlatform();
  button->SetFocusPainter(views::Painter::CreateSolidFocusPainter(
      message_center::kFocusBorderColor, gfx::Insets(1, 2, 2, 2)));

  button->SetInkDropMode(views::ImageButton::InkDropMode::ON);
  button->set_has_ink_drop_action_on_click(true);
  button->set_animate_on_state_change(true);
  button->set_ink_drop_base_color(SkColorSetRGB(0, 0, 0));
}

// "Roboto-Medium, 14sp" is specified in the mock.
gfx::FontList GetTextFontList() {
  gfx::Font default_font;
  int font_size_delta = kTextFontSize - default_font.GetFontSize();
  gfx::Font font = default_font.Derive(font_size_delta, gfx::Font::NORMAL,
                                       gfx::Font::Weight::MEDIUM);
  DCHECK_EQ(kTextFontSize, font.GetFontSize());
  return gfx::FontList(font);
}

views::Separator* CreateVerticalSeparator() {
  views::Separator* separator = new views::Separator;
  separator->SetPreferredHeight(24);
  separator->SetColor(kButtonSeparatorColor);
  separator->SetBorder(views::CreateEmptyBorder(12, 0, 12, 0));
  return separator;
}

}  // namespace

// MessageCenterButtonBar /////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
MessageCenterButtonBar::MessageCenterButtonBar(
    MessageCenterView* message_center_view,
    MessageCenter* message_center,
    NotifierSettingsProvider* notifier_settings_provider,
    bool settings_initially_visible,
    const base::string16& title)
    : message_center_view_(message_center_view),
      message_center_(message_center),
      back_arrow_(nullptr),
      notification_label_(nullptr),
      button_container_(nullptr),
      close_all_button_(nullptr),
      settings_button_(nullptr),
      quiet_mode_button_(nullptr) {
  SetPaintToLayer();
  SetBackground(views::CreateSolidBackground(
      MessageCenterView::kButtonBarBackgroundColor));

  notification_label_ = new views::Label(title);
  notification_label_->SetAutoColorReadabilityEnabled(false);
  notification_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  notification_label_->SetEnabledColor(kTextColor);
  notification_label_->SetFontList(GetTextFontList());
  AddChildView(notification_label_);

  button_container_ = new views::View;
  button_container_->SetLayoutManager(
      new views::BoxLayout(views::BoxLayout::kHorizontal));

  close_all_button_ = new views::ImageButton(this);
  close_all_button_->SetImage(
      Button::STATE_NORMAL,
      gfx::CreateVectorIcon(kNotificationCenterClearAllIcon, kButtonSize,
                            kActiveButtonColor));
  close_all_button_->SetImage(
      Button::STATE_DISABLED,
      gfx::CreateVectorIcon(kNotificationCenterClearAllIcon, kButtonSize,
                            kInactiveButtonColor));
  close_all_button_->SetTooltipText(l10n_util::GetStringUTF16(
      IDS_ASH_MESSAGE_CENTER_CLEAR_ALL_BUTTON_TOOLTIP));
  SetDefaultButtonStyles(close_all_button_);
  button_container_->AddChildView(close_all_button_);
  button_container_->AddChildView(CreateVerticalSeparator());

  quiet_mode_button_ = new views::ToggleImageButton(this);
  quiet_mode_button_->SetImage(
      Button::STATE_NORMAL,
      gfx::CreateVectorIcon(kNotificationCenterDoNotDisturbOffIcon, kButtonSize,
                            kInactiveButtonColor));
  // This is OK because ImageSkia will be copied in ToggleImageButton.
  gfx::ImageSkia quiet_mode_toggle_icon = gfx::CreateVectorIcon(
      kNotificationCenterDoNotDisturbOnIcon, kButtonSize, kActiveButtonColor);
  quiet_mode_button_->SetToggledImage(Button::STATE_NORMAL,
                                      &quiet_mode_toggle_icon);
  quiet_mode_button_->SetTooltipText(l10n_util::GetStringUTF16(
      IDS_ASH_MESSAGE_CENTER_QUIET_MODE_BUTTON_TOOLTIP));
  SetDefaultButtonStyles(quiet_mode_button_);
  button_container_->AddChildView(quiet_mode_button_);
  button_container_->AddChildView(CreateVerticalSeparator());

  settings_button_ = new views::ImageButton(this);
  settings_button_->SetImage(
      Button::STATE_NORMAL,
      gfx::CreateVectorIcon(kNotificationCenterSettingsIcon, kButtonSize,
                            kActiveButtonColor));
  settings_button_->SetTooltipText(l10n_util::GetStringUTF16(
      IDS_ASH_MESSAGE_CENTER_SETTINGS_BUTTON_TOOLTIP));
  SetDefaultButtonStyles(settings_button_);
  button_container_->AddChildView(settings_button_);

  back_arrow_ = new views::ImageButton(this);
  back_arrow_->SetImage(Button::STATE_NORMAL,
                        gfx::CreateVectorIcon(kNotificationCenterCollapseIcon,
                                              kButtonSize, kActiveButtonColor));
  SetDefaultButtonStyles(back_arrow_);
  button_container_->AddChildView(back_arrow_);

  SetCloseAllButtonEnabled(!settings_initially_visible);
  SetBackArrowVisible(settings_initially_visible);
  ViewVisibilityChanged();
}

void MessageCenterButtonBar::ViewVisibilityChanged() {
  views::GridLayout* layout = views::GridLayout::CreateAndInstall(this);
  views::ColumnSet* column = layout->AddColumnSet(0);
  constexpr int kFooterLeftMargin = 18;
  column->AddPaddingColumn(0, kFooterLeftMargin);

  // Column for the label "Notifications".
  column->AddColumn(views::GridLayout::LEADING, views::GridLayout::CENTER, 0.0f,
                    views::GridLayout::USE_PREF, 0, 0);

  // Fills in the remaining space between "Notifications" and buttons.
  column->AddPaddingColumn(1.0f, 18);

  // The button area column.
  column->AddColumn(views::GridLayout::LEADING, views::GridLayout::CENTER, 0.0f,
                    views::GridLayout::USE_PREF, 0, 0);

  constexpr int kFooterTopMargin = 4;
  layout->AddPaddingRow(0, kFooterTopMargin);
  layout->StartRow(0, 0, kButtonSize);
  layout->AddView(notification_label_);
  layout->AddView(button_container_);
  constexpr int kFooterBottomMargin = 4;
  layout->AddPaddingRow(0, kFooterBottomMargin);

  quiet_mode_button_->SetToggled(message_center()->IsQuietMode());
}

MessageCenterButtonBar::~MessageCenterButtonBar() {}

void MessageCenterButtonBar::SetSettingsAndQuietModeButtonsEnabled(
    bool enabled) {
  settings_button_->SetEnabled(enabled);
  quiet_mode_button_->SetEnabled(enabled);
}

void MessageCenterButtonBar::SetCloseAllButtonEnabled(bool enabled) {
  if (close_all_button_)
    close_all_button_->SetEnabled(enabled);
}

views::Button* MessageCenterButtonBar::GetCloseAllButtonForTest() const {
  return close_all_button_;
}

views::Button* MessageCenterButtonBar::GetQuietModeButtonForTest() const {
  return quiet_mode_button_;
}

views::Button* MessageCenterButtonBar::GetSettingsButtonForTest() const {
  return settings_button_;
}

void MessageCenterButtonBar::SetBackArrowVisible(bool visible) {
  if (back_arrow_)
    back_arrow_->SetVisible(visible);
  ViewVisibilityChanged();
  Layout();
}

void MessageCenterButtonBar::SetTitle(const base::string16& title) {
  notification_label_->SetText(title);
}

void MessageCenterButtonBar::SetButtonsVisible(bool visible) {
  settings_button_->SetVisible(visible);
  quiet_mode_button_->SetVisible(visible);

  if (close_all_button_)
    close_all_button_->SetVisible(visible);

  ViewVisibilityChanged();
  Layout();
}

void MessageCenterButtonBar::ChildVisibilityChanged(views::View* child) {
  InvalidateLayout();
}

void MessageCenterButtonBar::ButtonPressed(views::Button* sender,
                                           const ui::Event& event) {
  if (sender == close_all_button_) {
    message_center_view()->ClearAllClosableNotifications();
  } else if (sender == settings_button_) {
    message_center_view()->SetSettingsVisible(true);
  } else if (sender == back_arrow_) {
    message_center_view()->SetSettingsVisible(false);
  } else if (sender == quiet_mode_button_) {
    if (message_center()->IsQuietMode())
      message_center()->SetQuietMode(false);
    else
      message_center()->EnterQuietModeWithExpire(base::TimeDelta::FromDays(1));
    quiet_mode_button_->SetToggled(message_center()->IsQuietMode());
  } else {
    NOTREACHED();
  }
}

}  // namespace ash
