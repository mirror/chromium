// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/test/spawned_test_server/test_server_config.h"

#include "base/base_paths.h"
#include "base/environment.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/json/json_reader.h"
#include "base/lazy_instance.h"
#include "base/path_service.h"
#include "base/values.h"
#include "build/build_config.h"
#include "url/gurl.h"

namespace net {

namespace {

base::FilePath GetTestServerConfigFilePath() {
  base::FilePath dir;
#if defined(OS_ANDROID)
  PathService::Get(base::DIR_ANDROID_EXTERNAL_STORAGE, &dir);
#elif defined(OS_FUCHSIA)
  dir = base::FilePath("/system");
#else
  PathService::Get(base::DIR_TEMP, &dir);
#endif
  return dir.Append("net-test-server-config");
}

static base::LazyInstance<TestServerConfig>::Leaky g_test_server_config =
    LAZY_INSTANCE_INITIALIZER;

}  // namespace

// static
TestServerConfig* TestServerConfig::Get() {
  return g_test_server_config.Pointer();
}

GURL TestServerConfig::GetSpawnerUrl(const std::string& command) {
  CHECK(!spawner_url_base_.empty())
      << "spawner_url_base is expected, but not set in test server config.";
  std::string url = spawner_url_base_ + "/" + command;
  GURL result = GURL(url);
  CHECK(result.is_valid()) << url;
  return result;
}

TestServerConfig::TestServerConfig() {
  base::FilePath config_path = GetTestServerConfigFilePath();

  // Use defaults if the file doesn't exists.
  if (!base::PathExists(config_path))
    return;

  std::string config_json;
  if (!ReadFileToString(config_path, &config_json)) {
    LOG(FATAL) << "Failed to read " << config_path.value();
    return;
  }

  std::unique_ptr<base::DictionaryValue> config =
      base::DictionaryValue::From(base::JSONReader::Read(config_json));
  if (!config) {
    LOG(FATAL) << "Failed to parse " << config_path.value();
    return;
  }

  if (!config->GetString("name", &name_)) {
    LOG(WARNING) << "port isn't specified in test server config.";
  }

  std::string address_str;
  if (config->GetString("address", &address_str)) {
    if (!address_.AssignFromIPLiteral(address_str)) {
      LOG(FATAL) << "Invalid address specified in test server config: "
                 << address_str;
    }
  } else {
    LOG(WARNING) << "address isn't specified in test server config.";
  }

  if (config->GetString("spawner_url_base", &spawner_url_base_)) {
    GURL url(spawner_url_base_);
    if (!url.is_valid()) {
      LOG(FATAL) << "Invalid spawner_url_base specified in test server config: "
                 << spawner_url_base_;
    } else {
      allowed_port_ =
          std::make_unique<ScopedPortException>(url.EffectiveIntPort());
    }
  }
}

TestServerConfig::~TestServerConfig() {}

}  // namespace net
