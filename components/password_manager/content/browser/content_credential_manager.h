// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_PASSWORD_MANAGER_CONTENT_BROWSER_CONTENT_CREDENTIAL_MANAGER_H_
#define COMPONENTS_PASSWORD_MANAGER_CONTENT_BROWSER_CONTENT_CREDENTIAL_MANAGER_H_

#include "components/password_manager/core/browser/credential_manager_impl.h"
#include "components/password_manager/core/common/credential_manager_types.h"
#include "content/public/browser/web_contents_observer.h"
#include "mojo/public/cpp/bindings/associated_binding.h"
#include "third_party/WebKit/public/platform/modules/credentialmanager/credential_manager.mojom.h"

class GURL;

namespace content {
class WebContents;
}

namespace password_manager {
class PasswordManagerClient;
struct CredentialInfo;

class ContentCredentialManager : public mojom::CredentialManager,
                                 public content::WebContentsObserver,
                                 public CredentialManagerImplDelegate {
 public:
  ContentCredentialManager(content::WebContents* web_contents,
                           PasswordManagerClient* client);
  ~ContentCredentialManager() override;

  void BindRequest(mojom::CredentialManagerAssociatedRequest request);
  bool HasBinding() const;
  void DisconnectBinding();

  // TODO(tgarbus): Methods IsZeroClickAllowed and GetSynthesizedFormForOrigin
  // are used only in unit tests. Remove them after the tests have been moved
  // to core.
  bool IsZeroClickAllowed() const;
  PasswordStore::FormDigest GetSynthesizedFormForOrigin() const;

  // mojom::CredentialManager methods:
  void Store(const CredentialInfo& credential, StoreCallback callback) override;
  void PreventSilentAccess(PreventSilentAccessCallback callback) override;
  void Get(CredentialMediationRequirement mediation,
           bool include_passwords,
           const std::vector<GURL>& federations,
           GetCallback callback) override;

  // CredentialManagerImplDelegate:
  GURL GetLastCommittedURL() const override;

 private:
  CredentialManagerImpl impl_;

  mojo::AssociatedBinding<mojom::CredentialManager> binding_;

  DISALLOW_COPY_AND_ASSIGN(ContentCredentialManager);
};

}  // namespace password_manager

#endif  // COMPONENTS_PASSWORD_MANAGER_CONTENT_BROWSER_CONTENT_CREDENTIAL_MANAGER_H_
