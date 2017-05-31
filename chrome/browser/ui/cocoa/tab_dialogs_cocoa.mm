// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/cocoa/tab_dialogs_cocoa.h"

#include "base/memory/ptr_util.h"
#import "chrome/browser/ui/cocoa/browser_window_cocoa.h"
#import "chrome/browser/ui/cocoa/browser_window_controller.h"
#import "chrome/browser/ui/cocoa/content_settings/collected_cookies_mac.h"
#import "chrome/browser/ui/cocoa/hung_renderer_controller.h"
#import "chrome/browser/ui/cocoa/passwords/passwords_bubble_cocoa.h"
#import "chrome/browser/ui/cocoa/profiles/profile_signin_confirmation_dialog_cocoa.h"
#import "chrome/browser/ui/cocoa/translate/translate_bubble_controller.h"
#include "chrome/browser/ui/cocoa/tab_dialogs_views_mac.h"
#import "chrome/browser/ui/cocoa/validation_message_bubble_cocoa.h"
#include "chrome/browser/ui/sync/profile_signin_confirmation_helper.h"
#include "chrome/browser/ui/translate/translate_bubble_model_impl.h"
#include "components/translate/core/browser/translate_manager.h"
#include "components/translate/core/browser/translate_ui_delegate.h"
#include "content/public/browser/web_contents.h"
#include "ui/base/material_design/material_design_controller.h"

// static
void TabDialogs::CreateForWebContents(content::WebContents* contents) {
  DCHECK(contents);

  if (!FromWebContents(contents)) {
    std::unique_ptr<TabDialogs> tab_dialogs =
        ui::MaterialDesignController::IsSecondaryUiMaterial()
            ? base::MakeUnique<TabDialogsViewsMac>(contents)
            : base::MakeUnique<TabDialogsCocoa>(contents);
    contents->SetUserData(UserDataKey(), std::move(tab_dialogs));
  }
}

TabDialogsCocoa::TabDialogsCocoa(content::WebContents* contents)
    : web_contents_(contents),
      translate_bubble_controller_(nullptr) {
  DCHECK(contents);
}

TabDialogsCocoa::~TabDialogsCocoa() {
}

gfx::NativeView TabDialogsCocoa::GetDialogParentView() const {
  // View hierarchy of the contents view:
  // NSView  -- switchView, same for all tabs
  // +- TabContentsContainerView  -- TabContentsController's view
  //    +- WebContentsViewCocoa
  //
  // Changing it? Do not forget to modify
  // -[TabStripController swapInTabAtIndex:] too.
  return [web_contents_->GetNativeView() superview];
}

void TabDialogsCocoa::ShowCollectedCookies() {
  // Deletes itself on close.
  new CollectedCookiesMac(web_contents_);
}

void TabDialogsCocoa::ShowHungRendererDialog(
    const content::WebContentsUnresponsiveState& unresponsive_state) {
  [HungRendererController showForWebContents:web_contents_];
}

void TabDialogsCocoa::HideHungRendererDialog() {
  [HungRendererController endForWebContents:web_contents_];
}

void TabDialogsCocoa::ShowProfileSigninConfirmation(
    Browser* browser,
    Profile* profile,
    const std::string& username,
    std::unique_ptr<ui::ProfileSigninConfirmationDelegate> delegate) {
  ProfileSigninConfirmationDialogCocoa::Show(browser, web_contents_, profile,
                                             username, std::move(delegate));
}

void TabDialogsCocoa::ShowManagePasswordsBubble(bool user_action) {
  ManagePasswordsBubbleCocoa::Show(web_contents_, user_action);
}

void TabDialogsCocoa::HideManagePasswordsBubble() {
  // The bubble is closed when it loses the focus.
}

ShowTranslateBubbleResult TabDialogsCocoa::ShowTranslateBubble(
    BrowserWindow* window,
    translate::TranslateStep step,
    translate::TranslateErrors::Type error_type,
    bool is_user_gesture) {
  BrowserWindowController* controller =
      static_cast<BrowserWindowCocoa*>(window)->cocoa_controller();
  // TODO(hajimehoshi): The similar logic exists at TranslateBubbleView::
  // ShowBubble. This should be unified.
  if (translate_bubble_controller_) {
    // When the user reads the advanced setting panel, the bubble should not be
    // changed because they are focusing on the bubble.
    if (translate_bubble_controller_.webContents == web_contents_ &&
        translate_bubble_controller_.model->GetViewState() ==
        TranslateBubbleModel::VIEW_STATE_ADVANCED) {
      return ShowTranslateBubbleResult::SUCCESS;
    }
    if (step != translate::TRANSLATE_STEP_TRANSLATE_ERROR) {
      TranslateBubbleModel::ViewState viewState =
          TranslateBubbleModelImpl::TranslateStepToViewState(step);
      [translate_bubble_controller_ switchView:viewState];
    } else {
      [translate_bubble_controller_ switchToErrorView:error_type];
    }
    return ShowTranslateBubbleResult::SUCCESS;
  }

  std::string sourceLanguage;
  std::string targetLanguage;
  ChromeTranslateClient::GetTranslateLanguages(
      web_contents_, &sourceLanguage, &targetLanguage);

  std::unique_ptr<translate::TranslateUIDelegate> uiDelegate(
      new translate::TranslateUIDelegate(
          ChromeTranslateClient::GetManagerFromWebContents(web_contents_)
              ->GetWeakPtr(),
          sourceLanguage, targetLanguage));
  std::unique_ptr<TranslateBubbleModel> model(
      new TranslateBubbleModelImpl(step, std::move(uiDelegate)));
  translate_bubble_controller_ =
      [[TranslateBubbleController alloc] initWithParentWindow:controller
                                                        model:std::move(model)
                                                  webContents:web_contents_];
  [translate_bubble_controller_ showWindow:nil];

  translate_closure_listener_.reset([[WindowClosureListener alloc]
      initWithWindow:[translate_bubble_controller_ window]
            callback:base::Bind(&TabDialogsCocoa::TranslateWindowDidClose,
                                base::Unretained(this))]);

  return ShowTranslateBubbleResult::SUCCESS;
}

base::WeakPtr<ValidationMessageBubble> TabDialogsCocoa::ShowValidationMessage(
    const gfx::Rect& anchor_in_root_view,
    const base::string16& main_text,
    const base::string16& sub_text) {
  return (new ValidationMessageBubbleCocoa(
      web_contents_, anchor_in_root_view, main_text, sub_text))->AsWeakPtr();
}

void TabDialogsCocoa::TranslateWindowDidClose() {
  LOG(ERROR) << "TranslateWindowDidClose";
  translate_closure_listener_.reset();
  translate_bubble_controller_ = nil;
}
