// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/at_exit.h"
#include "base/command_line.h"
#include "base/message_loop/message_loop.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"
#include "net/base/ip_endpoint.h"
#include "net/test/tcp_socket_proxy.h"

const char kPortsSwitch[] = "ports";
const char kRemoteAddressSwitch[] = "remote-address";

int main(int argc, char** argv) {
  base::CommandLine::Init(argc, argv);
  base::AtExitManager at_exit;

  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();

  if (!command_line->HasSwitch(kPortsSwitch)) {
    LOG(ERROR) << "--" << kPortsSwitch << " was not specified.";
    return 1;
  }

  std::vector<std::string> ports_strings =
      base::SplitString(command_line->GetSwitchValueASCII(kPortsSwitch), ",",
                        base::TRIM_WHITESPACE, base::SPLIT_WANT_NONEMPTY);
  std::vector<int> ports;

  for (auto& port_str : ports_strings) {
    int port;
    if (!base::StringToInt(port_str, &port) || port <= 0 || port > 65535) {
      LOG(ERROR) << "Invalid value specified for --" << kPortsSwitch << ": "
                 << port_str;
      return 1;
    }
    ports.push_back(port);
  }

  if (ports.empty()) {
    LOG(ERROR) << "At least one port must be specified with --" << kPortsSwitch;
    return 1;
  }

  if (!command_line->HasSwitch(kRemoteAddressSwitch)) {
    LOG(ERROR) << "--" << kRemoteAddressSwitch << " was not specified.";
    return 1;
  }

  std::string remote_address_str =
      command_line->GetSwitchValueASCII(kRemoteAddressSwitch);
  net::IPAddress remote_address;
  if (!remote_address.AssignFromIPLiteral(remote_address_str)) {
    LOG(ERROR) << "Invalid value specified for --" << kRemoteAddressSwitch
               << ": " << remote_address_str;
    return 1;
  }

  base::MessageLoopForIO message_loop;

  std::vector<std::unique_ptr<net::TcpSocketProxy>> proxies;

  for (int port : ports) {
    auto test_server_proxy =
        std::make_unique<net::TcpSocketProxy>(message_loop.task_runner(), port);
    LOG(INFO) << "Listening on port " << test_server_proxy->local_port();
    test_server_proxy->Start(net::IPEndPoint(remote_address, port));
    proxies.push_back(std::move(test_server_proxy));
  }

  // Run the message loop indefinitely.
  base::RunLoop().Run();
}