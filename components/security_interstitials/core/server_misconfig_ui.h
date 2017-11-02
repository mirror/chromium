// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SECURITY_INTERSTITIALS_CORE_SERVER_MISCONFIG_UI_H_
#define COMPONENTS_SECURITY_INTERSTITIALS_CORE_SERVER_MISCONFIG_UI_H_

#include "base/macros.h"
#include "base/time/time.h"
#include "base/values.h"
#include "components/security_interstitials/core/controller_client.h"
#include "components/ssl_errors/error_classification.h"
#include "net/ssl/ssl_info.h"
#include "url/gurl.h"

namespace security_interstitials {

// Provides UI for SSL errors caused by server misconfigurations.
class ServerMisconfigUi {
 public:
  ServerMisconfigUi(const net::SSLInfo& ssl_info);
  ~ServerMisconfigUi();

  void PopulateStringsForHTML(base::DictionaryValue* load_time_data);

  const net::SSLInfo ssl_info_;

 private:
  // const net::SSLInfo& ssl_info_;

  DISALLOW_COPY_AND_ASSIGN(ServerMisconfigUi);
};

}  // namespace security_interstitials

#endif  // COMPONENTS_SECURITY_INTERSTITIALS_CORE_SERVER_MISCONFIG_UI_H_
