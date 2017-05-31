// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_SEARCH_LOCAL_NTP_SOURCE_H_
#define CHROME_BROWSER_SEARCH_LOCAL_NTP_SOURCE_H_

#include <memory>
#include <string>
#include <vector>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/optional.h"
#include "base/scoped_observer.h"
#include "base/time/time.h"
#include "content/public/browser/url_data_source.h"

class LocalNtpSourceImpl;
class Profile;

// Serves HTML and resources for the local new tab page i.e.
// chrome-search://local-ntp/local-ntp.html
class LocalNtpSource : public content::URLDataSource {
 public:
  explicit LocalNtpSource(Profile* profile);

 private:
  ~LocalNtpSource() override;

  // content::URLDataSource implementation.
  std::string GetSource() const override;
  void StartDataRequest(
      const std::string& path,
      const content::ResourceRequestInfo::WebContentsGetter& wc_getter,
      const content::URLDataSource::GotDataCallback& callback) override;
  std::string GetMimeType(const std::string& path) const override;
  bool AllowCaching() const override;
  bool ShouldServiceRequest(const GURL& url,
                            content::ResourceContext* resource_context,
                            int render_process_id) const override;
  std::string GetContentSecurityPolicyScriptSrc() const override;
  std::string GetContentSecurityPolicyChildSrc() const override;

  // Holds IO thread-affine data and logic. Deleted on the IO thread.
  std::unique_ptr<LocalNtpSourceImpl> ntp_source_;

  DISALLOW_COPY_AND_ASSIGN(LocalNtpSource);
};

#endif  // CHROME_BROWSER_SEARCH_LOCAL_NTP_SOURCE_H_
