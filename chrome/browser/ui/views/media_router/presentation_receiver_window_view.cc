// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/media_router/presentation_receiver_window_view.h"

#include "base/logging.h"
#include "chrome/browser/password_manager/chrome_password_manager_client.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ssl/security_state_tab_helper.h"
#include "chrome/browser/translate/chrome_translate_client.h"
#include "chrome/browser/ui/autofill/chrome_autofill_client.h"
#include "chrome/browser/ui/passwords/manage_passwords_ui_controller.h"
#include "chrome/browser/ui/search/search_tab_helper.h"
#include "chrome/browser/ui/tab_dialogs.h"
#include "components/toolbar/toolbar_model_impl.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/common/content_constants.h"
#include "ui/views/controls/webview/webview.h"
#include "ui/views/layout/grid_layout.h"

PresentationReceiverWindowView::PresentationReceiverWindowView(
    Controller* controller)
    : controller_(controller),
      toolbar_model_(
          std::make_unique<ToolbarModelImpl>(this,
                                             content::kMaxURLDisplayChars)),
      command_updater_(this) {
  DCHECK(controller);
}

PresentationReceiverWindowView::~PresentationReceiverWindowView() = default;

void PresentationReceiverWindowView::Init() {
  auto* const web_contents = GetWebContents();
  DCHECK(web_contents);

  SecurityStateTabHelper::CreateForWebContents(web_contents);
  ChromeTranslateClient::CreateForWebContents(web_contents);
  autofill::ChromeAutofillClient::CreateForWebContents(web_contents);
  ChromePasswordManagerClient::CreateForWebContentsWithAutofillClient(
      web_contents,
      autofill::ChromeAutofillClient::FromWebContents(web_contents));
  ManagePasswordsUIController::CreateForWebContents(web_contents);
  SearchTabHelper::CreateForWebContents(web_contents);
  TabDialogs::CreateForWebContents(web_contents);

  auto* profile =
      Profile::FromBrowserContext(web_contents->GetBrowserContext());
  auto* web_view = new views::WebView(profile);
  web_view->SetWebContents(web_contents);
  location_bar_view_ =
      new LocationBarView(nullptr, profile, &command_updater_, this, true);

  auto* grid = views::GridLayout::CreateAndInstall(this);
  auto* column_set = grid->AddColumnSet(0);
  column_set->AddColumn(views::GridLayout::FILL, views::GridLayout::FILL, 1,
                        views::GridLayout::USE_PREF, 0, 0);

  grid->StartRow(0, 0);
  grid->AddView(location_bar_view_);

  grid->StartRow(1, 0);
  grid->AddView(web_view);

  location_bar_view_->Init();
}

void PresentationReceiverWindowView::UpdateLocationBar() {
  DCHECK(location_bar_view_);
  location_bar_view_->Update(nullptr);
}

content::WebContents* PresentationReceiverWindowView::GetWebContents() {
  return controller_->GetWebContents();
}

ToolbarModel* PresentationReceiverWindowView::GetToolbarModel() {
  return toolbar_model_.get();
}

const ToolbarModel* PresentationReceiverWindowView::GetToolbarModel() const {
  return toolbar_model_.get();
}

ContentSettingBubbleModelDelegate*
PresentationReceiverWindowView::GetContentSettingBubbleModelDelegate() {
  NOTREACHED();
  return nullptr;
}

void PresentationReceiverWindowView::ExecuteCommandWithDisposition(
    int id,
    WindowOpenDisposition disposition) {
  NOTREACHED();
}

content::WebContents* PresentationReceiverWindowView::GetActiveWebContents()
    const {
  return controller_->GetWebContents();
}

void PresentationReceiverWindowView::WindowClosing() {
  controller_->WindowClosing();
}

base::string16 PresentationReceiverWindowView::GetWindowTitle() const {
  return title_;
}
