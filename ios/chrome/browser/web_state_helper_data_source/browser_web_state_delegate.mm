// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/web_state_helper_data_source/browser_web_state_delegate.h"

#include <memory>

#include "ios/chrome/browser/browser_state/chrome_browser_state.h"
#import "ios/chrome/browser/overlays/overlay_service.h"
#import "ios/chrome/browser/overlays/overlay_service_factory.h"
#import "ios/chrome/browser/web_state_list/web_state_list.h"
#import "ios/chrome/browser/web_state_list/web_state_opener.h"
#import "ios/clean/chrome/browser/ui/dialogs/context_menu/context_menu_dialog_coordinator.h"
#import "ios/clean/chrome/browser/ui/dialogs/context_menu/context_menu_dialog_request.h"
#import "ios/clean/chrome/browser/ui/dialogs/http_auth_dialogs/http_auth_dialog_coordinator.h"
#import "ios/clean/chrome/browser/ui/dialogs/http_auth_dialogs/http_auth_dialog_request.h"
#import "ios/clean/chrome/browser/ui/dialogs/java_script_dialogs/java_script_dialog_overlay_presenter.h"
#import "ios/web/public/navigation_manager.h"
#import "ios/web/public/web_state/web_state.h"
#include "url/gurl.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

DEFINE_BROWSER_USER_DATA_KEY(BrowserWebStateDelegate);

BrowserWebStateDelegate::BrowserWebStateDelegate(Browser* browser)
    : browser_state_(browser->browser_state()),
      web_state_list_(&browser->web_state_list()),
      overlay_service_(OverlayServiceFactory::GetInstance()->GetForBrowserState(
          browser->browser_state())) {
  DCHECK(browser);
  JavaScriptDialogOverlayPresenter::CreateForBrowser(browser);
  java_script_dialog_presenter_ =
      JavaScriptDialogOverlayPresenter::FromBrowser(browser);
}

BrowserWebStateDelegate::~BrowserWebStateDelegate() {}

#pragma mark - WebStateDelegate

web::WebState* BrowserWebStateDelegate::CreateNewWebState(
    web::WebState* source,
    const GURL& url,
    const GURL& opener_url,
    bool initiated_by_user) {
  // TODO(crbug.com/760863): Implement popup blocking.
  web::WebState* child_web_state = AddChildWebState(true);
  web::NavigationManager::WebLoadParams load_params(url);
  load_params.is_renderer_initiated = YES;
  child_web_state->GetNavigationManager()->LoadURLWithParams(load_params);
  return child_web_state;
}

void BrowserWebStateDelegate::CloseWebState(web::WebState* source) {
  // Only allow a web page to close itself if it was opened by DOM, or if there
  // are no NavigationItems.
  DCHECK(source->HasOpener() ||
         !source->GetNavigationManager()->GetItemCount());
  web_state_list_->CloseWebStateAt(web_state_list_->GetIndexOfWebState(source));
}

web::WebState* BrowserWebStateDelegate::OpenURLFromWebState(
    web::WebState* source,
    const web::WebState::OpenURLParams& params) {
  web::WebState* web_state = nullptr;
  if (params.disposition == WindowOpenDisposition::NEW_FOREGROUND_TAB ||
      params.disposition == WindowOpenDisposition::NEW_BACKGROUND_TAB ||
      params.disposition == WindowOpenDisposition::NEW_POPUP) {
    web_state = AddChildWebState(params.disposition !=
                                 WindowOpenDisposition::NEW_BACKGROUND_TAB);
  } else if (params.disposition == WindowOpenDisposition::CURRENT_TAB) {
    web_state = source;
  } else {
    NOTIMPLEMENTED();
    return nullptr;
  }

  web::NavigationManager::WebLoadParams loadParams(params.url);
  loadParams.referrer = params.referrer;
  loadParams.transition_type = params.transition;
  loadParams.is_renderer_initiated = params.is_renderer_initiated;
  web_state->GetNavigationManager()->LoadURLWithParams(loadParams);
  return web_state;
}

void BrowserWebStateDelegate::HandleContextMenu(
    web::WebState* source,
    const web::ContextMenuParams& params) {
  ContextMenuDialogRequest* request =
      [ContextMenuDialogRequest requestWithParams:params];
  ContextMenuDialogCoordinator* contextMenu =
      [[ContextMenuDialogCoordinator alloc] initWithRequest:request];
  overlay_service_->ShowOverlayForWebState(contextMenu, source);
}

void BrowserWebStateDelegate::ShowRepostFormWarningDialog(
    web::WebState* source,
    const base::Callback<void(bool)>& callback) {
  callback.Run(true);
}

web::JavaScriptDialogPresenter*
BrowserWebStateDelegate::GetJavaScriptDialogPresenter(web::WebState* source) {
  return java_script_dialog_presenter_;
}

void BrowserWebStateDelegate::OnAuthRequired(
    web::WebState* source,
    NSURLProtectionSpace* protection_space,
    NSURLCredential* proposed_credential,
    const AuthCallback& callback) {
  HTTPAuthDialogRequest* request = [HTTPAuthDialogRequest
      requestWithWebState:source
          protectionSpace:protection_space
               credential:proposed_credential
                 callback:^(NSString* _Nullable username,
                            NSString* _Nullable password) {
                   callback.Run(username, password);
                 }];
  HTTPAuthDialogCoordinator* authDialog =
      [[HTTPAuthDialogCoordinator alloc] initWithRequest:request];
  overlay_service_->ShowOverlayForWebState(authDialog, source);
}

#pragma mark - Private

web::WebState* BrowserWebStateDelegate::AddChildWebState(bool activate) {
  web::WebState::CreateParams create_params(browser_state_);
  create_params.created_with_opener = true;
  std::unique_ptr<web::WebState> child_web_state =
      web::WebState::Create(create_params);
  int index = web_state_list_->InsertWebState(
      WebStateList::kInvalidIndex, std::move(child_web_state),
      WebStateList::INSERT_INHERIT_OPENER, WebStateOpener(nullptr));
  DCHECK_NE(index, WebStateList::kInvalidIndex);
  if (activate)
    web_state_list_->ActivateWebStateAt(index);
  return web_state_list_->GetWebStateAt(index);
}
