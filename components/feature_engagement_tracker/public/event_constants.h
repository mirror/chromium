// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_FEATURE_ENGAGEMENT_TRACKER_PUBLIC_EVENT_CONSTANTS_H_
#define COMPONENTS_FEATURE_ENGAGEMENT_TRACKER_PUBLIC_EVENT_CONSTANTS_H_

namespace feature_engagement_tracker {

// All the events declared below are the string names
// of deferred onboarding events for the New Tab.

// The user has opened a new tab
extern const char kNewTabOpened[];

// The user has interacted with the omnibox.
extern const char kOmniboxInteraction[];

// The user has accumulated 2 hours of active session time.
extern const char kSessionTime[];

}  // namespace feature_engagement_tracker

#endif  // COMPONENTS_FEATURE_ENGAGEMENT_TRACKER_PUBLIC_EVENT_CONSTANTS_H_
