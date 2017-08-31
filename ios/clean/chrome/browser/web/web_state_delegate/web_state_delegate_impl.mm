// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/clean/chrome/browser/web/web_state_delegate/web_state_delegate_impl.h"

#include <memory>

#include "ios/chrome/browser/browser_state/chrome_browser_state.h"
#import "ios/chrome/browser/web_state_list/web_state_list.h"
#import "ios/chrome/browser/web_state_list/web_state_opener.h"
#import "ios/clean/chrome/browser/ui/dialogs/context_menu/context_menu_dialog_coordinator.h"
#import "ios/clean/chrome/browser/ui/dialogs/context_menu/context_menu_dialog_request.h"
#import "ios/clean/chrome/browser/ui/dialogs/java_script_dialogs/java_script_dialog_overlay_presenter.h"
#import "ios/clean/chrome/browser/ui/overlays/overlay_service.h"
#import "ios/clean/chrome/browser/ui/overlays/overlay_service_factory.h"
#import "ios/web/public/navigation_manager.h"
#import "ios/web/public/web_state/web_state.h"
#include "url/gurl.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

DEFINE_BROWSER_USER_DATA_KEY(WebStateDelegateImpl);

namespace {
// Attaches |delegate| to |web_state|.
void AttachDelegate(web::WebStateDelegate* delegate, web::WebState* web_state) {
  DCHECK(!web_state->GetDelegate());
  web_state->SetDelegate(delegate);
}
// Detaches |delegate| to |web_state|.
void DetachDelegate(web::WebStateDelegate* delegate, web::WebState* web_state) {
  DCHECK_EQ(web_state->GetDelegate(), delegate);
  web_state->SetDelegate(nullptr);
}
}  // namespace

WebStateDelegateImpl::WebStateDelegateImpl(Browser* browser)
    : browser_state_(browser->browser_state()),
      web_state_list_(&browser->web_state_list()),
      attached_(false),
      overlay_service_(OverlayServiceFactory::GetInstance()->GetForBrowserState(
          browser->browser_state())) {
  DCHECK(browser);
  JavaScriptDialogOverlayPresenter::CreateForBrowser(browser);
  java_script_dialog_presenter_ =
      JavaScriptDialogOverlayPresenter::FromBrowser(browser);
}

WebStateDelegateImpl::~WebStateDelegateImpl() {}

#pragma mark - Public

void WebStateDelegateImpl::Attach() {
  if (attached_)
    return;
  for (int index = 0; index < web_state_list_->count(); ++index) {
    AttachDelegate(this, web_state_list_->GetWebStateAt(index));
  }
  web_state_list_->AddObserver(this);
  attached_ = true;
}

void WebStateDelegateImpl::Detach() {
  if (!attached_)
    return;
  for (int index = 0; index < web_state_list_->count(); ++index) {
    DetachDelegate(this, web_state_list_->GetWebStateAt(index));
  }
  web_state_list_->RemoveObserver(this);
  attached_ = false;
}

#pragma mark - WebStateDelegate

web::WebState* WebStateDelegateImpl::CreateNewWebState(web::WebState* source,
                                                       const GURL& url,
                                                       const GURL& opener_url,
                                                       bool initiated_by_user) {
  // TODO: implement popup blocking.
  // Create a child WebState with an opener.
  web::WebState::CreateParams create_params(browser_state_);
  create_params.created_with_opener = true;
  std::unique_ptr<web::WebState> child_web_state =
      web::WebState::Create(create_params);
  // Perform a blank, renderer-initiated load.
  web::NavigationManager::WebLoadParams load_params(GURL::EmptyGURL());
  load_params.is_renderer_initiated = true;
  child_web_state->GetNavigationManager()->LoadURLWithParams(load_params);
  int index = web_state_list_->InsertWebState(
      WebStateList::kInvalidIndex, std::move(child_web_state),
      WebStateList::INSERT_INHERIT_OPENER, WebStateOpener(nullptr));
  DCHECK_NE(index, WebStateList::kInvalidIndex);
  return web_state_list_->GetWebStateAt(index);
}

void WebStateDelegateImpl::CloseWebState(web::WebState* source) {
  // Only allow a web page to close itself if it was opened by DOM, or if there
  // are no NavigationItems.
  DCHECK(source->HasOpener() ||
         !source->GetNavigationManager()->GetItemCount());
  web_state_list_->CloseWebStateAt(web_state_list_->GetIndexOfWebState(source));
}

web::WebState* WebStateDelegateImpl::OpenURLFromWebState(
    web::WebState* source,
    const web::WebState::OpenURLParams& params) {
  if (params.disposition == WindowOpenDisposition::CURRENT_TAB) {
    web::NavigationManager::WebLoadParams loadParams(params.url);
    loadParams.referrer = params.referrer;
    loadParams.transition_type = params.transition;
    loadParams.is_renderer_initiated = params.is_renderer_initiated;
    source->GetNavigationManager()->LoadURLWithParams(loadParams);
    return source;
  }
  // TODO: Implement other tab dispositions.
  return nil;
}

void WebStateDelegateImpl::HandleContextMenu(
    web::WebState* source,
    const web::ContextMenuParams& params) {
  ContextMenuDialogRequest* request =
      [ContextMenuDialogRequest requestWithParams:params];
  ContextMenuDialogCoordinator* contextMenu =
      [[ContextMenuDialogCoordinator alloc] initWithRequest:request];
  overlay_service_->ShowOverlayForWebState(contextMenu, source);
}

void WebStateDelegateImpl::ShowRepostFormWarningDialog(
    web::WebState* source,
    const base::Callback<void(bool)>& callback) {
  callback.Run(true);
}

web::JavaScriptDialogPresenter*
WebStateDelegateImpl::GetJavaScriptDialogPresenter(web::WebState* source) {
  return java_script_dialog_presenter_;
}

void WebStateDelegateImpl::OnAuthRequired(
    web::WebState* source,
    NSURLProtectionSpace* protection_space,
    NSURLCredential* proposed_credential,
    const AuthCallback& callback) {
  callback.Run(nil, nil);
}

#pragma mark - WebStateListObserver

void WebStateDelegateImpl::WebStateInsertedAt(WebStateList* web_state_list,
                                              web::WebState* web_state,
                                              int index,
                                              bool activating) {
  DCHECK(attached_);
  AttachDelegate(this, web_state);
}

void WebStateDelegateImpl::WebStateReplacedAt(WebStateList* web_state_list,
                                              web::WebState* old_web_state,
                                              web::WebState* new_web_state,
                                              int index) {
  DCHECK(attached_);
  DetachDelegate(this, old_web_state);
  AttachDelegate(this, new_web_state);
}

void WebStateDelegateImpl::WebStateDetachedAt(WebStateList* web_state_list,
                                              web::WebState* web_state,
                                              int index) {
  DCHECK(attached_);
  DetachDelegate(this, web_state);
}
