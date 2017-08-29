// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_SUBRESOURCE_FILTER_SUBRESOURCE_FILTER_FAILSAFE_H_
#define CHROME_BROWSER_SUBRESOURCE_FILTER_SUBRESOURCE_FILTER_FAILSAFE_H_

#include "base/macros.h"

class GURL;
class Profile;

namespace subresource_filter {
struct Configuration;
}  // namespace subresource_filter

class SubresourceFilterFailsafe {
 public:
  explicit SubresourceFilterFailsafe(Profile* profile);
  ~SubresourceFilterFailsafe();

  void OnDidShowUI(const GURL& url,
                   const subresource_filter::Configuration& matching_config);

 private:
  void RemoveConfiguration(const subresource_filter::Configuration& config);

  // Must outlive this class.
  Profile* profile_;

  DISALLOW_COPY_AND_ASSIGN(SubresourceFilterFailsafe);
};

#endif  // CHROME_BROWSER_SUBRESOURCE_FILTER_SUBRESOURCE_FILTER_FAILSAFE_H_
