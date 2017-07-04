// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_APP_LIST_SPEECH_AUTH_HELPER_H_
#define CHROME_BROWSER_UI_APP_LIST_SPEECH_AUTH_HELPER_H_

#include <memory>
#include <string>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "components/signin/core/browser/signin_manager.h"
#include "google_apis/gaia/oauth2_token_service.h"

class Profile;

namespace base {
class Clock;
}

namespace app_list {

// SpeechAuthHelper is a helper class to generate oauth tokens for audio
// history. This encalsulates generating and refreshing the auth token for a
// given profile. All functions should be called on the UI thread.
class SpeechAuthHelper : public OAuth2TokenService::Consumer,
                         public SigninManagerBase::Observer {
 public:
  SpeechAuthHelper(Profile* profile, base::Clock* clock);
  ~SpeechAuthHelper() override;

  // Returns the current auth token. If the auth service is not available, or
  // there was a failure in obtaining a token, return the empty string.
  std::string GetToken() const;

  // Returns the OAuth scope associated with the auth token.
  std::string GetScope() const;

 private:
  // Overridden from OAuth2TokenService::Consumer:
  void OnGetTokenSuccess(const OAuth2TokenService::Request* request,
                         const std::string& access_token,
                         const base::Time& expiration_time) override;
  void OnGetTokenFailure(const OAuth2TokenService::Request* request,
                         const GoogleServiceAuthError& error) override;

  // Overridden from SigninManagerBase::Observer:
  void OnAuthenticatedAccountStateChanged(
      const std::string& authenticated_account_id,
      const SigninManagerBase::Event& event) override;

  void ScheduleTokenFetch(const base::TimeDelta& fetch_delay);
  void FetchAuthToken();

  base::Clock* clock_;
  SigninManagerBase* signin_manager_;
  std::string authenticated_account_id_;
  std::string auth_token_;
  std::unique_ptr<OAuth2TokenService::Request> auth_token_request_;

  base::WeakPtrFactory<SpeechAuthHelper> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(SpeechAuthHelper);
};

}  // namespace app_list

#endif  // CHROME_BROWSER_UI_APP_LIST_SPEECH_AUTH_HELPER_H_
