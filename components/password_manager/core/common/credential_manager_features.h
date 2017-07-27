// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_PASSWORD_MANAGER_CORE_COMMON_CREDENTIAL_MANAGER_FEATURES_H_
#define COMPONENTS_PASSWORD_MANAGER_CORE_COMMON_CREDENTIAL_MANAGER_FEATURES_H_

#include "base/feature_list.h"
#include "build/build_config.h"

namespace credential_manager {

namespace features {

#if defined(OS_IOS)
// Used to control the state of the Credential Manager API feature.
extern const base::Feature kCredentialManager;
#endif

}  // namespace features

}  // namespace credential_manager

#endif  // COMPONENTS_PASSWORD_MANAGER_CORE_COMMON_CREDENTIAL_MANAGER_FEATURES_H_
