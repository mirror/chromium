// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_TEST_SPAWNED_TEST_SERVER_TEST_SERVER_CONFIG_H_
#define NET_TEST_SPAWNED_TEST_SERVER_TEST_SERVER_CONFIG_H_

#include <string>

#include "base/macros.h"
#include "net/base/ip_address.h"

class GURL;

namespace net {

// TestServerConfig is responsible for loading test server configuration from a
// file. Expected config location depends on platform:
//   - Android: DIR_ANDROID_EXTERNAL_STORAGE/net-test-server-config
//   - Fuchsia: /system/net-test-server-config
//   - other platforms: DIR_TEMP/net-test-server-config
//
// If the config file doesn't exist then the default configuration is used. By
// default the server is started locally on localhost, 127.0.0.1.
//
// If the config file exists then it must be stored in the following format:
//   {
//     'name': 'localhost',
//     'address': '127.0.0.1',
//     'spawner_url_base': 'http://localhost:5000'
//   }
//
// 'spawner_url_base' specified base URL to connect to the test server spawner
// responsible for starting and stopping test server. Currently spawner is used
// only on Android and Fuchsia. 'name' and 'address' parameters specify hostname
// and IP address for the test server. If test server spawner isn't used
// these should be set to 'localhost' and '127.0.0.1' (because
// net::LocalTestServer can run test server only on localhost).
class TestServerConfig {
 public:
  TestServerConfig();
  ~TestServerConfig();

  // Returns TestServerConfig singleton.
  static TestServerConfig* Get();

  // Hostname to use to connect to the testserver.
  const std::string& name() { return name_; }

  // IP address to use used to connect to the testserver.
  const IPAddress& address() { return address_; }

  // GURL for the test server spawner.
  GURL GetSpawnerUrl(const std::string& command);

 private:
  // Defaults that can be overridden with a config file.
  std::string name_ = "localhost";
  IPAddress address_ = {127, 0, 0, 1};

  std::string spawner_url_base_;

  DISALLOW_COPY_AND_ASSIGN(TestServerConfig);
};

}  // namespace net

#endif  // NET_TEST_SPAWNED_TEST_SERVER_TEST_SERVER_CONFIG_H_
