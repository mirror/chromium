// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/os_crypt/key_storage_config_linux.h"

#include <utility>

#include "base/lazy_instance.h"

namespace {

base::LazyInstance<std::unique_ptr<os_crypt::Config>>::Leaky g_config =
    LAZY_INSTANCE_INITIALIZER;

}  // namespace

namespace os_crypt {

Config::Config() {}
Config::~Config() {}

// static
Config* Config::GetInstance() {
  return g_config.Get().get();
}

void Config::SetInstance(std::unique_ptr<Config> config) {
  g_config.Get() = std::move(config);
}

}  // namespace os_crypt
