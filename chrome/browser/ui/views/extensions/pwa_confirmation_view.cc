// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/extensions/pwa_confirmation_view.h"

#include "base/callback_helpers.h"
#include "base/memory/ptr_util.h"
#include "base/strings/string16.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "build/build_config.h"
#include "chrome/browser/extensions/extension_util.h"
#include "chrome/browser/ui/views/extensions/web_app_info_image_source.h"
#include "chrome/browser/ui/views/harmony/chrome_layout_provider.h"
#include "chrome/grit/generated_resources.h"
#include "components/constrained_window/constrained_window_views.h"
#include "components/strings/grit/components_strings.h"
#include "content/public/browser/web_contents.h"
#include "extensions/common/constants.h"
#include "ui/accessibility/ax_node_data.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/gfx/image/image_skia.h"
#include "ui/gfx/image/image_skia_source.h"
#include "ui/views/controls/image_view.h"
#include "ui/views/controls/label.h"
#include "ui/views/layout/grid_layout.h"
#include "ui/views/layout/layout_provider.h"
#include "ui/views/widget/widget.h"
#include "ui/views/window/dialog_client_view.h"

namespace {

// Minimum width of the the bubble.
const int kMinBubbleWidth = 300;

}  // namespace

PWAConfirmationView::PWAConfirmationView(
    const WebApplicationInfo& web_app_info,
    chrome::AppInstallationAcceptanceCallback callback)
    : web_app_info_(web_app_info), callback_(std::move(callback)) {
  base::TrimWhitespace(web_app_info_.title, base::TRIM_ALL,
                       &web_app_info_.title);
  web_app_info_.open_as_window = true;

  CreateDialog(web_app_info_.title,
               base::UTF8ToUTF16(web_app_info_.app_url.GetOrigin().spec()),
               web_app_info_.icons);
}

PWAConfirmationView::~PWAConfirmationView() {}

ui::ModalType PWAConfirmationView::GetModalType() const {
  return ui::MODAL_TYPE_WINDOW;
}

base::string16 PWAConfirmationView::GetWindowTitle() const {
  return l10n_util::GetStringUTF16(IDS_ADD_TO_OS_LAUNCH_SURFACE_BUBBLE_TITLE);
}

bool PWAConfirmationView::ShouldShowCloseButton() const {
  return false;
}

void PWAConfirmationView::WindowClosing() {
  if (!callback_.is_null())
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

bool PWAConfirmationView::IsDialogButtonEnabled(ui::DialogButton button) const {
  return button == ui::DIALOG_BUTTON_OK ? !web_app_info_.title.empty() : true;
}

gfx::Size PWAConfirmationView::GetMinimumSize() const {
  gfx::Size size(views::DialogDelegateView::CalculatePreferredSize());
  size.SetToMax(gfx::Size(kMinBubbleWidth, 0));
  return size;
}

void PWAConfirmationView::CreateDialog(
    const base::string16& title,
    const base::string16& origin,
    const std::vector<WebApplicationInfo::IconInfo>& icons) {
  views::GridLayout* layout = views::GridLayout::CreateAndInstall(this);
  const ChromeLayoutProvider* layout_provider = ChromeLayoutProvider::Get();
  set_margins(layout_provider->GetDialogInsetsForContentType(views::CONTROL,
                                                             views::CONTROL));
  const int column_set_id = 0;

  views::ColumnSet* column_set = layout->AddColumnSet(column_set_id);
  column_set->AddColumn(views::GridLayout::FILL, views::GridLayout::CENTER, 0,
                        views::GridLayout::USE_PREF, 0, 0);
  column_set->AddPaddingColumn(0,
                               layout_provider->GetDistanceMetric(
                                   views::DISTANCE_RELATED_CONTROL_HORIZONTAL));
  constexpr int label_width = 320;
  column_set->AddColumn(views::GridLayout::FILL, views::GridLayout::CENTER, 0,
                        views::GridLayout::FIXED, label_width, 0);

  const int icon_size = layout_provider->IsHarmonyMode()
                            ? extension_misc::EXTENSION_ICON_SMALL
                            : extension_misc::EXTENSION_ICON_MEDIUM;
  views::ImageView* icon_image_view = new views::ImageView();
  gfx::Size image_size(icon_size, icon_size);
  gfx::ImageSkia image(
      base::MakeUnique<WebAppInfoImageSource>(icon_size, icons), image_size);
  icon_image_view->SetImageSize(image_size);
  icon_image_view->SetImage(image);

  layout->StartRow(0, column_set_id);
  layout->AddView(icon_image_view, 1, 2);  // Spans both title and origin lines.

  // Label showing the title of the app.
  views::Label* title_label = new views::Label(title);
  layout->AddView(title_label, 1, 1, views::GridLayout::LEADING,
                  views::GridLayout::CENTER);

  layout->StartRow(0, column_set_id);
  layout->SkipColumns(2);  // Skip image and padding columns.
  views::Label* origin_label = new views::Label(origin);
  layout->AddView(origin_label, 1, 1, views::GridLayout::LEADING,
                  views::GridLayout::CENTER);

  chrome::RecordDialogCreation(chrome::DialogIdentifier::PWA_CONFIRMATION);
}

namespace chrome {

void ShowPWAInstallDialog(gfx::NativeWindow parent,
                          const WebApplicationInfo& web_app_info,
                          AppInstallationAcceptanceCallback callback) {
  constrained_window::CreateBrowserModalDialogViews(
      new PWAConfirmationView(web_app_info, std::move(callback)), parent)
      ->Show();
}

}  // namespace chrome
