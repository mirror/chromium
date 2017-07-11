// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_OS_CRYPT_KEY_STORAGE_CONFIG_LINUX_H_
#define COMPONENTS_OS_CRYPT_KEY_STORAGE_CONFIG_LINUX_H_

#include <memory>
#include <string>

#include "base/files/file_path.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/single_thread_task_runner.h"

namespace os_crypt {

// A container for all the initialisation parameters for OSCrypt. Since OSCrypt
// is a static interface, the configuration is also stored a static instance.
struct Config {
 public:
  Config();
  ~Config();

  // Get the static config instance.
  static Config* GetInstance();
  // Set the static config instance.
  static void SetInstance(std::unique_ptr<Config> config_ptr);

  // Force OSCrypt to use a specific linux password store.
  std::string store;
  // The product name to use for permission prompts.
  std::string product_name;
  // A runner on the main thread for gnome-keyring to be called from.
  // TODO(crbug/466975): Libsecret and KWallet don't need this. We can remove
  // this when we stop supporting keyring.
  scoped_refptr<base::SingleThreadTaskRunner> main_thread_runner;
  // Controls whether preference on using or ignoring backends is used.
  bool should_use_preference;
  // Preferences are stored in a separate file in the user data directory.
  base::FilePath user_data_path;

 private:
  DISALLOW_COPY_AND_ASSIGN(Config);
};

}  // namespace os_crypt

#endif  // COMPONENTS_OS_CRYPT_KEY_STORAGE_CONFIG_LINUX_H_
