// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/extensions/hosted_app_error_view.h"

#include <memory>
#include <utility>

#include "base/strings/utf_string_conversions.h"
#include "chrome/grit/chromium_strings.h"
#include "chrome/grit/generated_resources.h"
#include "content/public/browser/web_contents.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/views/background.h"
#include "ui/views/controls/button/md_text_button.h"
#include "ui/views/controls/label.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/widget/widget.h"

HostedAppErrorView::HostedAppErrorView(
    content::WebContents* web_contents,
    OnWebContentsReparentedCallback on_web_contents_reparented)
    : extensions::HostedAppError(web_contents,
                                 std::move(on_web_contents_reparented)) {
  SetBackground(views::CreateThemedSolidBackground(
      this, ui::NativeTheme::kColorId_DialogBackground));

  auto* layout = new views::BoxLayout(views::BoxLayout::kVertical);
  layout->set_cross_axis_alignment(
      views::BoxLayout::CROSS_AXIS_ALIGNMENT_CENTER);
  layout->set_main_axis_alignment(views::BoxLayout::MAIN_AXIS_ALIGNMENT_CENTER);
  SetLayoutManager(layout);

  // TODO(ortuno): Use proper string in resources.
  title_ = new views::Label(
      base::UTF8ToUTF16("There is a problem with the current app."));
  ui::ResourceBundle& rb = ui::ResourceBundle::GetSharedInstance();
  title_->SetFontList(rb.GetFontList(ui::ResourceBundle::LargeFont));
  AddChildView(title_);

  // TODO(ortuno): Use proper string in resource.
  action_button_ = views::MdTextButton::CreateSecondaryUiBlueButton(
      this, l10n_util::GetStringUTF16(IDS_OPEN_IN_CHROME));
  AddChildView(action_button_);

  views::Widget::InitParams hosted_app_error_params(
      views::Widget::InitParams::TYPE_CONTROL);

  // It is not possible to create a native_widget_win that has no parent in
  // and later re-parent it.
  hosted_app_error_params.parent = web_contents->GetNativeView();

  set_owned_by_client();

  views::Widget* hosted_app_error = new views::Widget();
  hosted_app_error->Init(hosted_app_error_params);
  hosted_app_error->SetContentsView(this);

  views::Widget::ReparentNativeView(hosted_app_error->GetNativeView(),
                                    web_contents->GetNativeView());
  gfx::Rect bounds = web_contents->GetContainerBounds();
  hosted_app_error->SetBounds(gfx::Rect(bounds.size()));
}

HostedAppErrorView::~HostedAppErrorView() {
  if (GetWidget())
    GetWidget()->Close();
}

void HostedAppErrorView::ButtonPressed(views::Button* sender,
                                       const ui::Event& event) {
  Proceed();
}

namespace extensions {

std::unique_ptr<HostedAppError> HostedAppError::Create(
    content::WebContents* web_contents,
    OnWebContentsReparentedCallback on_web_contents_reparented) {
  return std::make_unique<HostedAppErrorView>(
      web_contents, std::move(on_web_contents_reparented));
}

}  // namespace extensions
