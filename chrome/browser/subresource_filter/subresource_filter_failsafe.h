// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_SUBRESOURCE_FILTER_SUBRESOURCE_FILTER_FAILSAFE_H_
#define CHROME_BROWSER_SUBRESOURCE_FILTER_SUBRESOURCE_FILTER_FAILSAFE_H_

#include <memory>

#include "base/macros.h"

class GURL;
class Profile;

namespace subresource_filter {
struct Configuration;
}  // namespace subresource_filter

// The failsafe is a class which monitors the the subresource filter component,
// and will remove configurations which seem broken, e.g. which activate or show
// UI on wrong sites.
class SubresourceFilterFailsafe {
 public:
  // Will return nullptr if the SubresourceFilterFailsafe feature is not
  // enabled. |profile| must outlive this class.
  static std::unique_ptr<SubresourceFilterFailsafe> MaybeCreate(
      Profile* profile);

  ~SubresourceFilterFailsafe();

  // This enum backs a histogram. Please only append items to the end and make
  // sure to update enums.xml with any changes.
  // TODO(csharrison): Consider adding more triggers.
  enum class Status : int {
    // It was safe to show the UI for this URL.
    kSafe,

    // The URL is a new tab page for this profile.
    kIsInstantNTP,

    // The URL is a non http/s URL. Note that this is only returned for non
    // http/s URLs that are *not* the new tab page.
    kIsNonHttp,

    kLast
  };

  // Returns the status of the given URL, which should be the URL of a main
  // frame navigation / document. If the status is kSafe, it is OK to procede,
  // otherwise the attempt should be aborted, and this class will remove the
  // matching configuration from the global ConfigurationList.
  Status CheckSafetyAndRemoveConfigurationIfUnsafe(
      const GURL& url,
      const subresource_filter::Configuration& matching_config);

 private:
  explicit SubresourceFilterFailsafe(Profile* profile);

  Status GetStatusForUrl(const GURL& url) const;

  // Removes the configuration from GetEnabledConfigurations.
  void RemoveConfiguration(const subresource_filter::Configuration& config);

  // Must outlive this class.
  Profile* profile_;

  DISALLOW_COPY_AND_ASSIGN(SubresourceFilterFailsafe);
};

#endif  // CHROME_BROWSER_SUBRESOURCE_FILTER_SUBRESOURCE_FILTER_FAILSAFE_H_
