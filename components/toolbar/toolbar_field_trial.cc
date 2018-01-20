// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/toolbar/toolbar_field_trial.h"

#include "base/feature_list.h"

namespace toolbar {

// Features used for EV UI removal experiment (https://crbug.com/803138).
const base::Feature kEVToSecure{"EVToSecure",
                                base::FEATURE_DISABLED_BY_DEFAULT};
const base::Feature kEVToLock{"EVToLock", base::FEATURE_DISABLED_BY_DEFAULT};
const base::Feature kSecureToLock{"SecureToLock",
                                  base::FEATURE_DISABLED_BY_DEFAULT};

}  // namespace toolbar
