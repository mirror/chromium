// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/password_manager/content/browser/content_credential_manager.h"

#include <utility>

#include "base/bind.h"
#include "base/memory/ptr_util.h"
#include "base/metrics/user_metrics.h"
#include "base/strings/string16.h"
#include "base/strings/utf_string_conversions.h"
#include "components/autofill/core/common/password_form.h"
#include "components/password_manager/content/browser/content_password_manager_driver.h"
#include "components/password_manager/content/browser/content_password_manager_driver_factory.h"
#include "components/password_manager/core/browser/android_affiliation/affiliated_match_helper.h"
#include "components/password_manager/core/browser/credential_manager_logger.h"
#include "components/password_manager/core/browser/form_fetcher_impl.h"
#include "components/password_manager/core/browser/form_saver.h"
#include "components/password_manager/core/browser/password_manager_client.h"
#include "components/password_manager/core/browser/password_manager_metrics_util.h"
#include "components/password_manager/core/browser/password_manager_util.h"
#include "components/password_manager/core/browser/password_store.h"
#include "components/password_manager/core/common/password_manager_pref_names.h"
#include "content/public/browser/web_contents.h"

namespace password_manager {

// ContentCredentialManager -------------------------------------------------

ContentCredentialManager::ContentCredentialManager(
    content::WebContents* web_contents,
    PasswordManagerClient* client)
    : WebContentsObserver(web_contents),
      client_(client),
      binding_(this),
      weak_factory_(this) {
  DCHECK(web_contents);
  impl_.reset(new CredentialManagerImpl(this, client_));
}

ContentCredentialManager::~ContentCredentialManager() {}

void ContentCredentialManager::BindRequest(
    mojom::CredentialManagerAssociatedRequest request) {
  DCHECK(!binding_.is_bound());
  binding_.Bind(std::move(request));

  // The browser side will close the message pipe on DidFinishNavigation before
  // the renderer side would be destroyed, and the renderer never explicitly
  // closes the pipe. So a connection error really means an error here, in which
  // case the renderer will try to reconnect when the next call to the API is
  // made. Make sure this implementation will no longer be bound to a broken
  // pipe once that happens, so the DCHECK above will succeed.
  binding_.set_connection_error_handler(base::Bind(
      &ContentCredentialManager::DisconnectBinding, base::Unretained(this)));
}

bool ContentCredentialManager::HasBinding() const {
  return binding_.is_bound();
}

void ContentCredentialManager::DisconnectBinding() {
  binding_.Close();
}

void ContentCredentialManager::Store(const CredentialInfo& credential,
                                     StoreCallback callback) {
  impl_.get()->Store(credential, std::move(callback));
}

void ContentCredentialManager::PreventSilentAccess(
    PreventSilentAccessCallback callback) {
  impl_.get()->PreventSilentAccess(std::move(callback));
}

void ContentCredentialManager::Get(CredentialMediationRequirement mediation,
                                   bool include_passwords,
                                   const std::vector<GURL>& federations,
                                   GetCallback callback) {
  impl_.get()->Get(mediation, include_passwords, federations,
                   std::move(callback));
}

GURL ContentCredentialManager::GetLastCommittedURL() const {
  return web_contents()->GetLastCommittedURL();
}

GURL ContentCredentialManager::GetOrigin() const {
  return web_contents()->GetLastCommittedURL().GetOrigin();
}

base::WeakPtr<PasswordManagerDriver> ContentCredentialManager::GetDriver() {
  ContentPasswordManagerDriverFactory* driver_factory =
      ContentPasswordManagerDriverFactory::FromWebContents(web_contents());
  DCHECK(driver_factory);
  PasswordManagerDriver* driver =
      driver_factory->GetDriverForFrame(web_contents()->GetMainFrame());
  return driver->AsWeakPtr();
}

}  // namespace password_manager
