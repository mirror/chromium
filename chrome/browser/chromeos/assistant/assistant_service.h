// Copyright (c) 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_ASSISTANT_ASSISTANT_SERVICE_H_
#define CHROME_BROWSER_CHROMEOS_ASSISTANT_ASSISTANT_SERVICE_H_

#include <memory>
#include <string>
#include <utility>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "content/public/browser/notification_observer.h"
#include "content/public/browser/notification_registrar.h"
#include "google_apis/gaia/oauth2_token_service.h"

class Profile;
class ProfileOAuth2TokenService;

namespace chromeos {
namespace assistant {

class AssistantServiceManager;

class AssistantService : public content::NotificationObserver,
                         public OAuth2TokenService::Observer,
                         public OAuth2TokenService::Consumer {
 public:
  AssistantService();
  ~AssistantService() override;

 private:
  // content::NotificationObserver overrides.
  void Observe(int type,
               const content::NotificationSource& source,
               const content::NotificationDetails& details) override;

  // OAuth2TokenService::Observer overrides.
  void OnRefreshTokenAvailable(const std::string& account_id) override;

  // OAuth2TokenService::Consumer overrides.
  void OnGetTokenSuccess(const OAuth2TokenService::Request* request,
                         const std::string& access_token,
                         const base::Time& expiration_time) override;
  void OnGetTokenFailure(const OAuth2TokenService::Request* request,
                         const GoogleServiceAuthError& error) override;

  void RefreshToken();

  content::NotificationRegistrar registrar_;
  ProfileOAuth2TokenService* token_service_;
  std::unique_ptr<OAuth2TokenService::Request> token_request_;
  Profile* profile_;
  std::string account_id_;

  std::unique_ptr<AssistantServiceManager> assistant_;

  base::WeakPtrFactory<AssistantService> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(AssistantService);
};

}  // namespace assistant
}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_ASSISTANT_ASSISTANT_SERVICE_H_
