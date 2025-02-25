// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/infobars/confirm_infobar.h"

#include <memory>
#include <utility>

#include "base/logging.h"
#include "build/build_config.h"
#include "chrome/browser/infobars/infobar_service.h"
#include "chrome/browser/ui/views/elevation_icon_setter.h"
#include "chrome/browser/ui/views/harmony/chrome_layout_provider.h"
#include "chrome/browser/ui/views_mode_controller.h"
#include "ui/base/material_design/material_design_controller.h"
#include "ui/base/window_open_disposition.h"
#include "ui/native_theme/native_theme.h"
#include "ui/views/controls/button/label_button.h"
#include "ui/views/controls/button/md_text_button.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/link.h"
#include "ui/views/view_properties.h"

// InfoBarService -------------------------------------------------------------

std::unique_ptr<infobars::InfoBar> InfoBarService::CreateConfirmInfoBar(
    std::unique_ptr<ConfirmInfoBarDelegate> delegate) {
#if defined(OS_MACOSX)
  if (views_mode_controller::IsViewsBrowserCocoa())
    return InfoBarService::CreateConfirmInfoBarCocoa(std::move(delegate));
#endif
  return std::make_unique<ConfirmInfoBar>(std::move(delegate));
}


// ConfirmInfoBar -------------------------------------------------------------

ConfirmInfoBar::ConfirmInfoBar(std::unique_ptr<ConfirmInfoBarDelegate> delegate)
    : InfoBarView(std::move(delegate)),
      label_(nullptr),
      ok_button_(nullptr),
      cancel_button_(nullptr),
      link_(nullptr) {
  // Always use the standard theme for the platform on infobars (infobars in
  // incognito should have the same appearance as normal infobars).
  SetNativeTheme(ui::NativeTheme::GetInstanceForNativeUi());
}

ConfirmInfoBar::~ConfirmInfoBar() {
  // Ensure |elevation_icon_setter_| is destroyed before |ok_button_|.
  elevation_icon_setter_.reset();
}

void ConfirmInfoBar::Layout() {
  InfoBarView::Layout();

  int x = StartX();
  Labels labels;
  labels.push_back(label_);
  labels.push_back(link_);
  AssignWidths(&labels, std::max(0, EndX() - x - NonLabelWidth()));

  ChromeLayoutProvider* layout_provider = ChromeLayoutProvider::Get();

  label_->SetPosition(gfx::Point(x, OffsetY(label_)));
  if (!label_->text().empty())
    x = label_->bounds().right() +
        layout_provider->GetDistanceMetric(
            views::DISTANCE_RELATED_LABEL_HORIZONTAL);

  if (ok_button_) {
    ok_button_->SetPosition(gfx::Point(x, OffsetY(ok_button_)));
    x = ok_button_->bounds().right() +
        layout_provider->GetDistanceMetric(
            views::DISTANCE_RELATED_BUTTON_HORIZONTAL);
  }

  if (cancel_button_)
    cancel_button_->SetPosition(gfx::Point(x, OffsetY(cancel_button_)));

  link_->SetPosition(gfx::Point(EndX() - link_->width(), OffsetY(link_)));
}

void ConfirmInfoBar::ViewHierarchyChanged(
    const ViewHierarchyChangedDetails& details) {
  if (details.is_add && details.child == this && (label_ == nullptr)) {
    ConfirmInfoBarDelegate* delegate = GetDelegate();
    label_ = CreateLabel(delegate->GetMessageText());
    AddViewToContentArea(label_);

    if (delegate->GetButtons() & ConfirmInfoBarDelegate::BUTTON_OK) {
      ok_button_ = CreateButton(ConfirmInfoBarDelegate::BUTTON_OK);
      ok_button_->SetProminent(true);
      if (delegate->OKButtonTriggersUACPrompt()) {
        elevation_icon_setter_.reset(new ElevationIconSetter(
            ok_button_,
            base::Bind(&ConfirmInfoBar::Layout, base::Unretained(this))));
      }
    }

    if (delegate->GetButtons() & ConfirmInfoBarDelegate::BUTTON_CANCEL) {
      cancel_button_ = CreateButton(ConfirmInfoBarDelegate::BUTTON_CANCEL);
      if (delegate->GetButtons() == ConfirmInfoBarDelegate::BUTTON_CANCEL)
        cancel_button_->SetProminent(true);
    }

    base::string16 link_text(delegate->GetLinkText());
    link_ = CreateLink(link_text, this);
    AddViewToContentArea(link_);
  }

  // This must happen after adding all other children so InfoBarView can ensure
  // the close button is the last child.
  InfoBarView::ViewHierarchyChanged(details);
}

void ConfirmInfoBar::ButtonPressed(views::Button* sender,
                                   const ui::Event& event) {
  if (!owner())
    return;  // We're closing; don't call anything, it might access the owner.
  ConfirmInfoBarDelegate* delegate = GetDelegate();
  if (sender == ok_button_) {
    if (delegate->Accept())
      RemoveSelf();
  } else if (sender == cancel_button_) {
    if (delegate->Cancel())
      RemoveSelf();
  } else {
    InfoBarView::ButtonPressed(sender, event);
  }
}

int ConfirmInfoBar::ContentMinimumWidth() const {
  return label_->GetMinimumSize().width() + link_->GetMinimumSize().width() +
      NonLabelWidth();
}

void ConfirmInfoBar::LinkClicked(views::Link* source, int event_flags) {
  if (!owner())
    return;  // We're closing; don't call anything, it might access the owner.
  DCHECK_EQ(link_, source);
  if (GetDelegate()->LinkClicked(ui::DispositionFromEventFlags(event_flags)))
    RemoveSelf();
}

ConfirmInfoBarDelegate* ConfirmInfoBar::GetDelegate() {
  return delegate()->AsConfirmInfoBarDelegate();
}

views::MdTextButton* ConfirmInfoBar::CreateButton(
    ConfirmInfoBarDelegate::InfoBarButton type) {
  auto* button =
      views::MdTextButton::Create(this, GetDelegate()->GetButtonLabel(type));
  button->SetProperty(
      views::kMarginsKey,
      new gfx::Insets(ChromeLayoutProvider::Get()->GetDistanceMetric(
                          DISTANCE_TOAST_CONTROL_VERTICAL),
                      0));
  AddViewToContentArea(button);
  button->SizeToPreferredSize();
  return button;
}

int ConfirmInfoBar::NonLabelWidth() const {
  ChromeLayoutProvider* layout_provider = ChromeLayoutProvider::Get();

  const int label_spacing = layout_provider->GetDistanceMetric(
      views::DISTANCE_RELATED_LABEL_HORIZONTAL);
  const int button_spacing = layout_provider->GetDistanceMetric(
      views::DISTANCE_RELATED_BUTTON_HORIZONTAL);

  int width = (label_->text().empty() || (!ok_button_ && !cancel_button_))
                  ? 0
                  : label_spacing;
  if (ok_button_)
    width += ok_button_->width() + (cancel_button_ ? button_spacing : 0);
  width += cancel_button_ ? cancel_button_->width() : 0;
  return width + ((link_->text().empty() || !width) ? 0 : label_spacing);
}
