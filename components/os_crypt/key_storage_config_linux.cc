// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/os_crypt/key_storage_config_linux.h"

#include <utility>

#include "base/memory/ptr_util.h"

namespace {

std::unique_ptr<os_crypt::Config>* GetConfigSingleton() {
  static auto singleton = base::MakeUnique<os_crypt::Config>();
  return &singleton;
}

}  // namespace

namespace os_crypt {

Config::Config() {}
Config::~Config() {}

// static
Config* Config::GetInstance() {
  return GetConfigSingleton()->get();
}

void Config::SetInstance(std::unique_ptr<Config> config) {
  *GetConfigSingleton() = std::move(config);
}

}  // namespace os_crypt
