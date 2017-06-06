// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_WEBSHARE_SHARE_TARGET_H_
#define CHROME_BROWSER_WEBSHARE_SHARE_TARGET_H_

#include <string>

#include "base/strings/string16.h"
#include "base/values.h"
#include "url/gurl.h"

// Represents a Web Share Target and its attributes. The attributes are usually
// retrieved from the share_target field in the site's manifest.
class ShareTarget {
 public:
  ShareTarget(GURL manifest_url, std::string name, std::string url_template);
  ~ShareTarget();

  // Move constructor
  ShareTarget(ShareTarget&& other) = default;

  // Move assigment
  ShareTarget& operator=(ShareTarget&& other) = default;

  // ShareTarget is move-only to avoid accidental copies. Clone() copies all
  // of ShareTarget's attributes so use sparingly.
  ShareTarget Clone() const;

  const std::string& name() const { return name_; }
  const GURL& manifest_url() const { return manifest_url_; }
  const std::string& url_template() const { return url_template_; }

  bool operator==(const ShareTarget& other) const;

 private:
  GURL manifest_url_;
  std::string name_;
  std::string url_template_;

  DISALLOW_COPY_AND_ASSIGN(ShareTarget);
};

// Used by gtest to print a readable output on test failures.
std::ostream& operator<<(std::ostream& out, const ShareTarget& target);

#endif  // CHROME_BROWSER_WEBSHARE_SHARE_TARGET_H_
