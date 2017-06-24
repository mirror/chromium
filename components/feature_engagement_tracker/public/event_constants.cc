// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/feature_engagement_tracker/public/event_constants.h"

namespace new_tab_feature_engagement_tracker {

#if defined(OS_WIN) || defined(OS_LINUX)
const char kNewTabOpened[] = "new_tab_opened";
const char kOmniboxInteraction[] = "omnibox_used";
const char kSessionTime[] = "session_time";
#endif  // defined(OS_WIN) || defined(OS_LINUX)

}  // namespace new_tab_feature_engagement_tracker
