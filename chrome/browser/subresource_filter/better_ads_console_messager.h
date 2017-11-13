// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_SUBRESOURCE_FILTER_BETTER_ADS_CONSOLE_MESSAGER_H_
#define CHROME_BROWSER_SUBRESOURCE_FILTER_BETTER_ADS_CONSOLE_MESSAGER_H_

#include <memory>

#include "base/macros.h"
#include "components/subresource_filter/content/browser/console_messager.h"

std::unique_ptr<subresource_filter::ConsoleMessager> MakeBetterAdsMessager(
    bool warning);

class BetterAdsConsoleMessager : public subresource_filter::ConsoleMessager {
 public:
  BetterAdsConsoleMessager();
  ~BetterAdsConsoleMessager() override;

 private:
  // subresource_filter::ConsoleMessager:
  void LogOnCommit(bool successfully_activated,
                   content::RenderFrameHost* rfh) override;
  void LogSubframeDisallowed(const GURL& subframe_url,
                             content::RenderFrameHost* rfh) override;
  bool ShouldLogSubresources() override;

  DISALLOW_COPY_AND_ASSIGN(BetterAdsConsoleMessager);
};

class BetterAdsWarningConsoleMessager
    : public subresource_filter::ConsoleMessager {
 public:
  BetterAdsWarningConsoleMessager();
  ~BetterAdsWarningConsoleMessager() override;

 private:
  // subresource_filter::ConsoleMessager:
  void LogOnCommit(bool successfully_activated,
                   content::RenderFrameHost* rfh) override;
  DISALLOW_COPY_AND_ASSIGN(BetterAdsWarningConsoleMessager);
};

#endif  // CHROME_BROWSER_SUBRESOURCE_FILTER_BETTER_ADS_CONSOLE_MESSAGER_H_
