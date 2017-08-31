// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CLEAN_CHROME_BROWSER_WEB_WEB_STATE_DELEGATE_WEB_STATE_DELEGATE_IMPL_H_
#define IOS_CLEAN_CHROME_BROWSER_WEB_WEB_STATE_DELEGATE_WEB_STATE_DELEGATE_IMPL_H_

#import "ios/chrome/browser/ui/browser_list/browser_user_data.h"
#import "ios/chrome/browser/web_state_list/web_state_list_observer.h"
#import "ios/web/public/web_state/web_state_delegate.h"

class OverlayService;
class WebStateList;
namespace web {
class BrowserState;
class JavaScriptDialogPresenter;
}  // namespace web

// An implementation of WebStateDelegate that uses OverlayService to show UI
// requested by WebStates.
class WebStateDelegateImpl : public BrowserUserData<WebStateDelegateImpl>,
                             public web::WebStateDelegate,
                             public WebStateListObserver {
 public:
  ~WebStateDelegateImpl() override;

  // Attaches or detaches the delegate to the Browser's WebStates.
  void Attach();
  void Detach();

 private:
  friend class BrowserUserData<WebStateDelegateImpl>;

  // Private constructor used by factory method.
  explicit WebStateDelegateImpl(Browser* browser);

  // WebStateDelegate:
  web::WebState* CreateNewWebState(web::WebState* source,
                                   const GURL& url,
                                   const GURL& opener_url,
                                   bool initiated_by_user) override;
  void CloseWebState(web::WebState* source) override;
  web::WebState* OpenURLFromWebState(
      web::WebState*,
      const web::WebState::OpenURLParams&) override;
  void HandleContextMenu(web::WebState* source,
                         const web::ContextMenuParams& params) override;
  void ShowRepostFormWarningDialog(
      web::WebState* source,
      const base::Callback<void(bool)>& callback) override;
  web::JavaScriptDialogPresenter* GetJavaScriptDialogPresenter(
      web::WebState* source) override;
  void OnAuthRequired(web::WebState* source,
                      NSURLProtectionSpace* protection_space,
                      NSURLCredential* proposed_credential,
                      const AuthCallback& callback) override;

  // WebStateListObserver:
  void WebStateInsertedAt(WebStateList* web_state_list,
                          web::WebState* web_state,
                          int index,
                          bool activating) override;
  void WebStateReplacedAt(WebStateList* web_state_list,
                          web::WebState* old_web_state,
                          web::WebState* new_web_state,
                          int index) override;
  void WebStateDetachedAt(WebStateList* web_state_list,
                          web::WebState* web_state,
                          int index) override;

  // The Browser's BrowserState.
  web::BrowserState* browser_state_;
  // The Browser's WebStateList.
  WebStateList* web_state_list_;
  // Whether this is attached as a delegate to the WebStates in
  // |web_state_list_|.
  bool attached_;
  // The JavaScriptDialogPresenter to provide to the WebStates.
  web::JavaScriptDialogPresenter* java_script_dialog_presenter_;
  // The OverlayService to use for presenting UI requested by the WebStates.
  OverlayService* overlay_service_;

  DISALLOW_COPY_AND_ASSIGN(WebStateDelegateImpl);
};

#endif  // IOS_CLEAN_CHROME_BROWSER_WEB_WEB_STATE_DELEGATE_WEB_STATE_DELEGATE_IMPL_H_
