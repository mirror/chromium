// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_UTILITY_MEDIA_ROUTER_DIAL_APP_INFO_PARSER_IMPL_H_
#define CHROME_UTILITY_MEDIA_ROUTER_DIAL_APP_INFO_PARSER_IMPL_H_

#include "base/macros.h"
#include "base/threading/thread_checker.h"
#include "chrome/common/media_router/mojo/dial_app_info_parser.mojom.h"

namespace media_router {

// Implementation of Mojo DialAppInfoParser. It accepts device
// description parsing request from browser process, and handles it in utility
// process. Class must be created and destroyed on utility thread.
class DialAppInfoParserImpl : public chrome::mojom::DialAppInfoParser {
 public:
  DialAppInfoParserImpl();
  ~DialAppInfoParserImpl() override;

  static void Create(chrome::mojom::DialAppInfoParserRequest request);

 private:
  friend class DialAppInfoParserImplTest;

  // extensions::mojom::DialAppInfoParser:
  void ParseDialAppInfo(const std::string& app_info_xml_data,
                        ParseDialAppInfoCallback callback) override;

  // Processes the result of getting device description. Returns valid
  // DialAppInfoPtr if processing succeeds; otherwise returns nullptr.
  // |xml|: The device description xml text.
  // |parsing_error|: Set by the method to an error value if parsing fails, or
  // NONE if parsing succeeds. Does not take ownership of |parsing_error|.
  chrome::mojom::DialAppInfoPtr Parse(
      const std::string& xml,
      chrome::mojom::DialAppInfoParsingError* parsing_error);

  THREAD_CHECKER(thread_checker_);

  DISALLOW_COPY_AND_ASSIGN(DialAppInfoParserImpl);
};

}  // namespace media_router

#endif  // CHROME_UTILITY_MEDIA_ROUTER_DIAL_APP_INFO_PARSER_IMPL_H_
