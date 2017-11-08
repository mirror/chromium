// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_MEDIA_ROUTER_DISCOVERY_DIAL_SAFE_DIAL_APP_INFO_PARSER_H_
#define CHROME_BROWSER_MEDIA_ROUTER_DISCOVERY_DIAL_SAFE_DIAL_APP_INFO_PARSER_H_

#include <memory>
#include <string>

#include "base/callback.h"
#include "base/macros.h"
#include "base/threading/thread_checker.h"
#include "chrome/common/media_router/mojo/dial_app_info_parser.mojom.h"
#include "content/public/browser/utility_process_mojo_client.h"

namespace media_router {

// SafeDialAppInfoParser parses the given app info XML file safely via a utility
// process. This class runs on IO thread.
class SafeDialAppInfoParser {
 public:
  // Callback function to be invoked when utility process finishes parsing
  // app info XML.
  // |mojo_app_info|: app info object. Empty if parsing fails.
  // |mojo_parsing_error|: error encountered while parsing DIAL app info XML.
  using AppInfoCallback = base::Callback<void(
      chrome::mojom::DialAppInfoPtr mojo_app_info,
      chrome::mojom::DialAppInfoParsingError mojo_parsing_error)>;

  SafeDialAppInfoParser();
  virtual ~SafeDialAppInfoParser();

  // Start parsing app info XML file in utility process.
  virtual void Start(const std::string& xml_text,
                     const AppInfoCallback& callback);

 private:
  // Utility client used to send app info parsing task to the utility process.
  std::unique_ptr<
      content::UtilityProcessMojoClient<chrome::mojom::DialAppInfoParser>>
      utility_process_mojo_client_;

  THREAD_CHECKER(thread_checker_);

  DISALLOW_COPY_AND_ASSIGN(SafeDialAppInfoParser);
};

}  // namespace media_router

#endif  // CHROME_BROWSER_MEDIA_ROUTER_DISCOVERY_DIAL_SAFE_DIAL_APP_INFO_PARSER_H_
