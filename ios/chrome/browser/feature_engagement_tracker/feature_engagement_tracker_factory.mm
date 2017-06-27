// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/chrome/browser/feature_engagement_tracker/feature_engagement_tracker_factory.h"

#include "base/memory/ptr_util.h"
#include "base/memory/ref_counted.h"
#include "base/memory/singleton.h"
#include "base/sequenced_task_runner.h"
#include "base/task_scheduler/post_task.h"
#include "components/feature_engagement_tracker/public/feature_engagement_tracker.h"
#include "components/keyed_service/ios/browser_state_dependency_manager.h"
#include "ios/chrome/browser/browser_state/browser_state_otr_helper.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state.h"
#include "ios/chrome/browser/chrome_constants.h"

// static
FeatureEngagementTrackerFactory*
FeatureEngagementTrackerFactory::GetInstance() {
  return base::Singleton<FeatureEngagementTrackerFactory>::get();
}

feature_engagement_tracker::FeatureEngagementTracker*
FeatureEngagementTrackerFactory::GetForBrowserState(
    ios::ChromeBrowserState* state) {
  return nullptr;
}

FeatureEngagementTrackerFactory::FeatureEngagementTrackerFactory()
    : BrowserStateKeyedServiceFactory(
          "FeatureEngagementTracker",
          BrowserStateDependencyManager::GetInstance()) {}

FeatureEngagementTrackerFactory::~FeatureEngagementTrackerFactory() = default;

std::unique_ptr<KeyedService>
FeatureEngagementTrackerFactory::BuildServiceInstanceFor(
    web::BrowserState* context) const {
  return nullptr;
}

web::BrowserState* FeatureEngagementTrackerFactory::GetBrowserStateToUse(
    web::BrowserState* context) const {
  return nullptr;
}
