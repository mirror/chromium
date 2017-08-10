// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_PREVIEWS_CORE_PREVIEWS_AMP_REDIRECTION_H_
#define COMPONENTS_PREVIEWS_CORE_PREVIEWS_AMP_REDIRECTION_H_

#include <map>
#include <memory>
#include <string>

#include "url/gurl.h"

namespace re2 {
class RE2;
}

namespace previews {

// Checks whether an URL is whitelisted for AMP redirection previews and
// implements the scheme used to convert URL to its AMP version.
class PreviewsAMPConverter {
 public:
  PreviewsAMPConverter();

  virtual ~PreviewsAMPConverter();

  // Returns true of |url| can be converted to its amp version. |new_amp_url|
  // will contain the AMP URL.
  bool GetAMPURL(const GURL& url, GURL* new_amp_url) const;

 private:
  // Contains the parameters for converting URL for one domain.
  struct AMPConverterEntry {
    AMPConverterEntry(const std::string& hostname,
                      std::unique_ptr<re2::RE2> pattern,
                      const std::string& hostname_amp,
                      const std::string& prefix,
                      const std::string& suffix,
                      const std::string& suffix_html);

    ~AMPConverterEntry();

    // Hostname the URL should match with.
    std::string hostname;

    // RE2 pattern the URL should match.
    std::unique_ptr<re2::RE2> pattern;

    // New AMP hostname for the URL, if any.
    std::string hostname_amp;

    // String to be prefixed to the URL location, if any.
    std::string prefix;

    // String to be suffixed to the URL location, before query string, if any.
    std::string suffix;

    // String to be suffixed to the URL location, before the .html extension, if
    // any.
    std::string suffix_html;
  };

  // Maps the hostname to its AMP conversion scheme.
  std::map<std::string, std::unique_ptr<AMPConverterEntry>> amp_converter_;
};

}  // namespace previews

#endif  // COMPONENTS_PREVIEWS_CORE_PREVIEWS_AMP_REDIRECTION_H_
