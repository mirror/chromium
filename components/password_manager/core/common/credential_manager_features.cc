// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/password_manager/core/common/credential_manager_features.h"

namespace credential_manager {

namespace features {

#if defined(OS_IOS)
const base::Feature kCredentialManager{"CredentialManager",
                                       base::FEATURE_DISABLED_BY_DEFAULT};
#endif

}  // namespace features

}  // namespace credential_manager
