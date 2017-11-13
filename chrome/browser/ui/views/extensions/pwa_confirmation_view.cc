// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/extensions/pwa_confirmation_view.h"

#include <memory>
#include <utility>

#include "base/callback_helpers.h"
#include "base/strings/string16.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/ui/views/extensions/web_app_info_image_source.h"
#include "chrome/browser/ui/views/harmony/chrome_layout_provider.h"
#include "chrome/browser/ui/views/harmony/chrome_typography.h"
#include "chrome/grit/generated_resources.h"
#include "components/constrained_window/constrained_window_views.h"
#include "components/strings/grit/components_strings.h"
#include "components/url_formatter/elide_url.h"
#include "extensions/common/constants.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/gfx/image/image_skia.h"
#include "ui/gfx/text_elider.h"
#include "ui/views/controls/image_view.h"
#include "ui/views/controls/label.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/style/typography.h"
#include "ui/views/widget/widget.h"
#include "ui/views/window/dialog_client_view.h"
#include "url/origin.h"

PWAConfirmationView::PWAConfirmationView(
    const WebApplicationInfo& web_app_info,
    chrome::AppInstallationAcceptanceCallback callback)
    : web_app_info_(web_app_info), callback_(std::move(callback)) {
  base::TrimWhitespace(web_app_info_.title, base::TRIM_ALL,
                       &web_app_info_.title);
  // PWAs should always be configured to open in a window.
  DCHECK(web_app_info_.open_as_window);

  InitializeView();

  chrome::RecordDialogCreation(chrome::DialogIdentifier::PWA_CONFIRMATION);
}

PWAConfirmationView::~PWAConfirmationView() {}

ui::ModalType PWAConfirmationView::GetModalType() const {
  return ui::MODAL_TYPE_CHILD;
}

base::string16 PWAConfirmationView::GetWindowTitle() const {
  return l10n_util::GetStringUTF16(IDS_ADD_TO_OS_LAUNCH_SURFACE_BUBBLE_TITLE);
}

bool PWAConfirmationView::ShouldShowCloseButton() const {
  return false;
}

void PWAConfirmationView::WindowClosing() {
  if (callback_)
    std::move(callback_).Run(false, web_app_info_);
}

bool PWAConfirmationView::Accept() {
  base::ResetAndReturn(&callback_).Run(true, web_app_info_);
  return true;
}

base::string16 PWAConfirmationView::GetDialogButtonLabel(
    ui::DialogButton button) const {
  return l10n_util::GetStringUTF16(button == ui::DIALOG_BUTTON_OK ? IDS_ADD
                                                                  : IDS_CANCEL);
}

namespace {

// Returns an ImageView containing the app icon.
std::unique_ptr<views::ImageView> CreateIconView(
    const std::vector<WebApplicationInfo::IconInfo>& icons) {
  gfx::ImageSkia image(
      std::make_unique<WebAppInfoImageSource>(kPWAConfirmationViewIconSize,
                                              icons),
      gfx::Size(kPWAConfirmationViewIconSize, kPWAConfirmationViewIconSize));

  std::unique_ptr<views::ImageView> icon_image_view =
      std::make_unique<views::ImageView>();
  icon_image_view->SetImage(image);
  return icon_image_view;
}

// Returns a label containing the app name.
std::unique_ptr<views::Label> CreateNameLabel(const base::string16& name,
                                              int label_width) {
  std::unique_ptr<views::Label> name_label = std::make_unique<views::Label>(
      name, CONTEXT_BODY_TEXT_LARGE, views::style::TextStyle::STYLE_PRIMARY);
  name_label->SetHorizontalAlignment(gfx::ALIGN_LEFT);

  base::string16 elided =
      gfx::ElideText(name, name_label->font_list(), label_width,
                     gfx::ElideBehavior::ELIDE_TAIL);

  name_label->SetText(elided);
  if (elided != name) {
    name_label->SetTooltipText(name);
  }
  return name_label;
}

// Returns a label containing the origin.
std::unique_ptr<views::Label> CreateOriginLabel(const url::Origin& origin,
                                                int label_width) {
  std::unique_ptr<views::Label> origin_label = std::make_unique<views::Label>(
      FormatOriginForSecurityDisplay(
          origin, url_formatter::SchemeDisplay::OMIT_HTTP_AND_HTTPS),
      CONTEXT_BODY_TEXT_SMALL, STYLE_SECONDARY);

  origin_label->SetAllowCharacterBreak(true);
  origin_label->SetMultiLine(true);
  origin_label->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  origin_label->SizeToFit(label_width);
  return origin_label;
}

}  // namespace

void PWAConfirmationView::InitializeView() {
  const ChromeLayoutProvider* layout_provider = ChromeLayoutProvider::Get();
  gfx::Insets margin_insets = layout_provider->GetDialogInsetsForContentType(
      views::CONTROL, views::CONTROL);
  set_margins(margin_insets);

  int icon_label_spacing = layout_provider->GetDistanceMetric(
      views::DISTANCE_RELATED_CONTROL_HORIZONTAL);
  SetLayoutManager(new views::BoxLayout(views::BoxLayout::kHorizontal,
                                        gfx::Insets(), icon_label_spacing));

  std::unique_ptr<views::ImageView> icon_view =
      CreateIconView(web_app_info_.icons);

  int icon_width = icon_view->GetImageBounds().width();
  this->AddChildView(icon_view.release());

  views::View* labels = new views::View();
  this->AddChildView(labels);
  labels->SetLayoutManager(new views::BoxLayout(views::BoxLayout::kVertical));

  int bubble_width = layout_provider->GetDistanceMetric(
      DISTANCE_MODAL_DIALOG_WIDTH_CONTAINING_MULTILINE_TEXT);

  int label_width =
      bubble_width - margin_insets.width() - icon_width - icon_label_spacing;

  labels->AddChildView(
      CreateNameLabel(web_app_info_.title, label_width).release());
  labels->AddChildView(
      CreateOriginLabel(url::Origin::Create(web_app_info_.app_url), label_width)
          .release());
}

namespace chrome {

void ShowPWAInstallDialog(content::WebContents* web_contents,
                          const WebApplicationInfo& web_app_info,
                          AppInstallationAcceptanceCallback callback) {
  constrained_window::CreateWebModalDialogViews(
      new PWAConfirmationView(web_app_info, std::move(callback)), web_contents)
      ->Show();
}

}  // namespace chrome
