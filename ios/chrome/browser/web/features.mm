// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/chrome/browser/web/features.h"

namespace ios {

// Feature flag for mailto: URL rewriting defaults to enabled. Change the
// default value here to turn off or on this feature.
const base::Feature kMailtoUrlRewriting{"MailtoUrlRewriting",
                                        base::FEATURE_DISABLED_BY_DEFAULT};

}  // namespace ios
