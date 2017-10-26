// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/password_manager/core/browser/ui/export_flow.h"

namespace password_manager {

ExportFlow::ExportFlow(CredentialProviderInterface* credential_provider)
    : credential_provider_(credential_provider) {}

ExportFlow::~ExportFlow() = default;

}  // namespace password_manager
