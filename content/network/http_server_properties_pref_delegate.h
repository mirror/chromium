// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_NETWORK_HTTP_SERVER_PROPERTIES_PREF_DELEGATE_H_
#define CONTENT_NETWORK_HTTP_SERVER_PROPERTIES_PREF_DELEGATE_H_

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "components/prefs/json_pref_store.h"
#include "components/prefs/pref_change_registrar.h"
#include "components/prefs/pref_service.h"
#include "net/http/http_server_properties.h"
#include "net/http/http_server_properties_manager.h"

namespace base {
class FilePath;
}  // namespace base

namespace content {

// Manages disk storage for a net::HttpServerPropertiesManager.
class HttpServerPropertiesPrefDelegate
    : public net::HttpServerPropertiesManager::PrefDelegate {
 public:
  explicit HttpServerPropertiesPrefDelegate(const base::FilePath& file_path);
  ~HttpServerPropertiesPrefDelegate() override;

  // net::HttpServerPropertiesManager::PrefDelegate implementation.
  bool HasServerProperties() override;
  const base::DictionaryValue& GetServerProperties() const override;
  void SetServerProperties(const base::DictionaryValue& value) override;
  void StartListeningForUpdates(const base::Closure& callback) override;
  void StopListeningForUpdates() override;

 private:
  std::unique_ptr<PrefService> pref_service_;
  PrefChangeRegistrar pref_change_registrar_;

  DISALLOW_COPY_AND_ASSIGN(HttpServerPropertiesPrefDelegate);
};

}  // namespace content

#endif  // CONTENT_NETWORK_HTTP_SERVER_PROPERTIES_PREF_DELEGATE_H_
