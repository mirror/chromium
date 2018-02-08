// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_ASSISTANT_ASSISTANT_BROWSER_PART_H_
#define CHROME_BROWSER_CHROMEOS_ASSISTANT_ASSISTANT_BROWSER_PART_H_

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "chromeos/services/assistant/service.h"
#include "components/signin/core/browser/account_info.h"
#include "content/public/browser/notification_observer.h"
#include "content/public/browser/notification_registrar.h"
#include "services/identity/public/cpp/account_state.h"
#include "services/identity/public/interfaces/identity_manager.mojom.h"

class Profile;

namespace chromeos {
namespace assistant {

class AssistantBrowserPart : public Service::AssistantServiceDelegate,
                             public content::NotificationObserver {
 public:
  AssistantBrowserPart();
  ~AssistantBrowserPart() override;

  // chromeos::assitant::AssistantServiceDelegate overrides:
  void RequestAccessToken(
      identity::mojom::IdentityManager::GetAccessTokenCallback callback)
      override;

 private:
  // content::NotificationObserver overrides:
  void Observe(int type,
               const content::NotificationSource& source,
               const content::NotificationDetails& details) override;

  void RefreshTokenIfNeeded();
  identity::mojom::IdentityManager* GetIdentityManager();

  void GetPrimaryAccountInfoCallback(
      const base::Optional<AccountInfo>& account_info,
      const identity::AccountState& account_state);

  Profile* profile_;
  content::NotificationRegistrar registrar_;
  identity::mojom::IdentityManager::GetAccessTokenCallback pending_callback_;
  identity::mojom::IdentityManagerPtr identity_manager_;

  base::WeakPtrFactory<AssistantBrowserPart> weak_factory_;
  DISALLOW_COPY_AND_ASSIGN(AssistantBrowserPart);
};

}  // namespace assistant
}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_ASSISTANT_ASSISTANT_BROWSER_PART_H_
