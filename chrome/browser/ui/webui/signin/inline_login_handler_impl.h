// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBUI_SIGNIN_INLINE_LOGIN_HANDLER_IMPL_H_
#define CHROME_BROWSER_UI_WEBUI_SIGNIN_INLINE_LOGIN_HANDLER_IMPL_H_

#include <memory>
#include <string>

#include "base/files/file_path.h"
#include "base/gtest_prod_util.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "chrome/browser/ui/sync/one_click_signin_sync_starter.h"
#include "chrome/browser/ui/webui/signin/finish_login.h"
#include "chrome/browser/ui/webui/signin/inline_login_handler.h"
#include "content/public/browser/web_contents_observer.h"

struct FinishCompleteLoginParams;

// Implementation for the inline login WebUI handler on desktop Chrome. Once
// CrOS migrates to the same webview approach as desktop Chrome, much of the
// code in this class should move to its base class |InlineLoginHandler|.
class InlineLoginHandlerImpl : public InlineLoginHandler,
                               public content::WebContentsObserver,
                               public InlineSigninHelperDelegate {
 public:
  InlineLoginHandlerImpl();
  ~InlineLoginHandlerImpl() override;

  using InlineLoginHandler::web_ui;
  using InlineLoginHandler::CloseDialogFromJavascript;

  Browser* GetDesktopBrowser();

  base::WeakPtr<InlineLoginHandlerImpl> GetWeakPtr() {
    return weak_factory_.GetWeakPtr();
  }

  // InlineSigninHelperDelegate:
  void SyncStarterCallback(
      OneClickSigninSyncStarter::SyncSetupResult result) override;
  // Closes the current tab and shows the account management view of the avatar
  // bubble if |show_account_management| is true.
  void CloseTab(bool show_account_management) override;
  void HandleLoginError(const std::string& error_msg,
                        const base::string16& email) override;
  void CloseLoginDialog() override;

 private:
  friend class InlineLoginUIBrowserTest;
  FRIEND_TEST_ALL_PREFIXES(InlineLoginUIBrowserTest, CanOfferNoProfile);
  FRIEND_TEST_ALL_PREFIXES(InlineLoginUIBrowserTest, CanOffer);
  FRIEND_TEST_ALL_PREFIXES(InlineLoginUIBrowserTest, CanOfferProfileConnected);
  FRIEND_TEST_ALL_PREFIXES(InlineLoginUIBrowserTest,
                           CanOfferUsernameNotAllowed);
  FRIEND_TEST_ALL_PREFIXES(InlineLoginUIBrowserTest, CanOfferWithRejectedEmail);
  FRIEND_TEST_ALL_PREFIXES(InlineLoginUIBrowserTest, CanOfferNoSigninCookies);

  // InlineLoginHandler overrides:
  void SetExtraInitParams(base::DictionaryValue& params) override;
  void CompleteLogin(const base::ListValue* args) override;

  static void FinishCompleteLogin(const FinishCompleteLoginParams& params,
                                  const GURL& current_url,
                                  Profile* profile,
                                  Profile::CreateStatus status);

  // content::WebContentsObserver implementation:
  void DidFinishNavigation(
      content::NavigationHandle* navigation_handle) override;

  // True if the user has navigated to untrusted domains during the signin
  // process.
  bool confirm_untrusted_signin_;

  base::WeakPtrFactory<InlineLoginHandlerImpl> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(InlineLoginHandlerImpl);
};

#endif  // CHROME_BROWSER_UI_WEBUI_SIGNIN_INLINE_LOGIN_HANDLER_IMPL_H_
