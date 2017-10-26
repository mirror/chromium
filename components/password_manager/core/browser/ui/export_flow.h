// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_UI_EXPORT_FLOW_H_
#define COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_UI_EXPORT_FLOW_H_

#include "base/macros.h"
#include "components/password_manager/core/browser/ui/credential_provider_interface.h"

namespace password_manager {

// This represents the controller for the UI flow of exporting passwords.
class ExportFlow {
 public:
  explicit ExportFlow(CredentialProviderInterface* credential_provider);

  // Store exported passwords to the export destination.
  virtual void Store() = 0;

 protected:
  virtual ~ExportFlow();

  CredentialProviderInterface* credential_provider() {
    return credential_provider_;
  }

 private:
  CredentialProviderInterface* const credential_provider_;

  DISALLOW_COPY_AND_ASSIGN(ExportFlow);
};

}  // namespace password_manager

#endif  // COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_UI_EXPORT_FLOW_H_
