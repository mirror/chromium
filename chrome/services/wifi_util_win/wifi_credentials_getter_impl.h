// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_SERVICES_WIFI_UTIL_WIN_WIFI_CREDENTIALS_GETTER_IMPL_H_
#define CHROME_SERVICES_WIFI_UTIL_WIN_WIFI_CREDENTIALS_GETTER_IMPL_H_

#include <memory>

#include "chrome/services/wifi_util_win/public/interfaces/wifi_credentials_getter.mojom.h"
#include "services/service_manager/public/cpp/service_context_ref.h"

namespace chrome {

class WiFiCredentialsGetterImpl : public chrome::mojom::WiFiCredentialsGetter {
 public:
  explicit WiFiCredentialsGetterImpl(
      std::unique_ptr<service_manager::ServiceContextRef> service_ref);
  ~WiFiCredentialsGetterImpl() override;

 private:
  // chrome::mojom::WiFiCredentialsGetter:
  void GetWiFiCredentials(const std::string& ssid,
                          GetWiFiCredentialsCallback callback) override;

  const std::unique_ptr<service_manager::ServiceContextRef> service_ref_;

  DISALLOW_COPY_AND_ASSIGN(WiFiCredentialsGetterImpl);
};

}  // namespace chrome

#endif  // CHROME_SERVICES_WIFI_UTIL_WIN_WIFI_CREDENTIALS_GETTER_IMPL_H_
