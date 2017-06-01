// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_WEBSHARE_SHARE_TARGET_H_
#define CHROME_BROWSER_WEBSHARE_SHARE_TARGET_H_

#include "base/strings/string16.h"
#include "base/values.h"
#include "url/gurl.h"

class ShareTarget {
 public:
  ShareTarget(GURL manifest_url, std::string name, std::string url_template);
  ~ShareTarget();

  ShareTarget(ShareTarget&& other) = default;
  ShareTarget& operator=(ShareTarget&& other) = default;

  const std::string& GetName() const { return name_; }
  const GURL& GetManifestURL() const { return manifest_url_; }
  const std::string& GetURLTemplate() const { return url_template_; }

  bool operator==(const ShareTarget& other) const;

 private:
  GURL manifest_url_;
  std::string name_;
  std::string url_template_;

  DISALLOW_COPY_AND_ASSIGN(ShareTarget);
};

#endif  // CHROME_BROWSER_WEBSHARE_SHARE_TARGET_H_
