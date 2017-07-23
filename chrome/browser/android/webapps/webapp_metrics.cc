// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/android/webapps/webapp_metrics.h"

#include "base/metrics/histogram_macros.h"

namespace webapp {

void RecordAddToHomescreenFromAppMenuType(AddToHomescreenType type) {
  UMA_HISTOGRAM_ENUMERATION("Webapp.MenuItemAddToHomescreen.Type", type,
                            AddToHomescreenType::COUNT);
}

}  // namespace webapp
