// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_WEB_STATE_HELPER_DATA_SOURCE_BROWSER_WEB_STATE_DELEGATE_H_
#define IOS_CHROME_BROWSER_WEB_STATE_HELPER_DATA_SOURCE_BROWSER_WEB_STATE_DELEGATE_H_

#import "ios/chrome/browser/ui/browser_list/browser_user_data.h"
#import "ios/web/public/web_state/web_state_delegate.h"

class OverlayService;
class WebStateList;
namespace web {
class BrowserState;
class JavaScriptDialogPresenter;
}  // namespace web

// An implementation of WebStateDelegate that uses OverlayService to show UI
// requested by WebStates.
class BrowserWebStateDelegate : public BrowserUserData<BrowserWebStateDelegate>,
                                public web::WebStateDelegate {
 public:
  ~BrowserWebStateDelegate() override;

 private:
  friend class BrowserUserData<BrowserWebStateDelegate>;

  // Private constructor used by factory method.
  explicit BrowserWebStateDelegate(Browser* browser);

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

  // Adds a WebState to |web_state_list_| using the active WebState as the
  // opener and |browser_state_| in the CreateParams.  Activates the new
  // WebState if |activate| is true.  Returns the newly created WebState.
  web::WebState* AddChildWebState(bool activate);

  // The Browser's BrowserState.
  web::BrowserState* browser_state_;
  // The Browser's WebStateList.
  WebStateList* web_state_list_;
  // The JavaScriptDialogPresenter to provide to the WebStates.
  web::JavaScriptDialogPresenter* java_script_dialog_presenter_;
  // The OverlayService to use for presenting UI requested by the WebStates.
  OverlayService* overlay_service_;

  DISALLOW_COPY_AND_ASSIGN(BrowserWebStateDelegate);
};

#endif  // IOS_CHROME_BROWSER_WEB_STATE_HELPER_DATA_SOURCE_BROWSER_WEB_STATE_DELEGATE_H_
