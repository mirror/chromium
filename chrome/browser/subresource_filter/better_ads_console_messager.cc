// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/subresource_filter/better_ads_console_messager.h"

#include <sstream>

#include "base/feature_list.h"
#include "base/logging.h"
#include "components/subresource_filter/core/browser/subresource_filter_constants.h"
#include "components/subresource_filter/core/browser/subresource_filter_features.h"
#include "content/public/browser/render_frame_host.h"

std::unique_ptr<subresource_filter::ConsoleMessager> MakeBetterAdsMessager(
    bool warning) {
  if (!base::FeatureList::IsEnabled(
          subresource_filter::kSafeBrowsingSubresourceFilterExperimentalUI)) {
    return nullptr;
  }

  if (warning)
    return std::make_unique<BetterAdsWarningConsoleMessager>();
  return std::make_unique<BetterAdsConsoleMessager>();
}

BetterAdsConsoleMessager::BetterAdsConsoleMessager() = default;
BetterAdsConsoleMessager::~BetterAdsConsoleMessager() = default;

void BetterAdsConsoleMessager::LogOnCommit(bool successfully_activated,
                                           content::RenderFrameHost* rfh) {
  if (successfully_activated) {
    rfh->AddMessageToConsole(content::CONSOLE_MESSAGE_LEVEL_WARNING,
                             subresource_filter::kActivationConsoleMessage);
  }
}

void BetterAdsConsoleMessager::LogSubframeDisallowed(
    const GURL& subframe_url,
    content::RenderFrameHost* rfh) {
  std::ostringstream oss(
      subresource_filter::kDisallowSubframeConsoleMessagePrefix);
  oss << subframe_url;
  oss << subresource_filter::kDisallowSubframeConsoleMessageSuffix;
  rfh->AddMessageToConsole(content::CONSOLE_MESSAGE_LEVEL_ERROR, oss.str());
}

bool BetterAdsConsoleMessager::ShouldLogSubresources() {
  return true;
}

BetterAdsWarningConsoleMessager::BetterAdsWarningConsoleMessager() = default;
BetterAdsWarningConsoleMessager::~BetterAdsWarningConsoleMessager() = default;

void BetterAdsWarningConsoleMessager::LogOnCommit(
    bool successfully_activated,
    content::RenderFrameHost* rfh) {
  rfh->AddMessageToConsole(
      content::CONSOLE_MESSAGE_LEVEL_WARNING,
      subresource_filter::kActivationWarningConsoleMessage);
}
