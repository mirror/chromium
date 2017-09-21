// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// A binary wrapper for QuicServer.  It listens forever on --port
// (default 6121) until it's killed or ctrl-cd to death.

#include <iostream>

#include "base/at_exit.h"
#include "base/command_line.h"
#include "base/logging.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "base/strings/string_number_conversions.h"
#include "net/base/ip_address.h"
#include "net/base/ip_endpoint.h"
#include "net/quic/chromium/crypto/proof_source_chromium.h"
#include "net/quic/core/quic_packets.h"
#include "net/tools/quic/quic_http_response_cache.h"
#include "net/tools/quic/quic_simple_server.h"

// The port the quic server will listen on.
int32_t FLAGS_port = 6121;
// Mode of operations: Quic reverse proxy (proxy) or cached response (cache)
std::string FLAGS_quic_mode = "cache";
// URL with http/https, IP address or host name and the port number of the
// backend server
std::string FLAGS_quic_proxy_backend_url;

std::unique_ptr<net::ProofSource> CreateProofSource(
    const base::FilePath& cert_path,
    const base::FilePath& key_path) {
  std::unique_ptr<net::ProofSourceChromium> proof_source(
      new net::ProofSourceChromium());
  CHECK(proof_source->Initialize(cert_path, key_path, base::FilePath()));
  return std::move(proof_source);
}

int main(int argc, char* argv[]) {
  base::AtExitManager exit_manager;
  base::MessageLoopForIO message_loop;

  base::CommandLine::Init(argc, argv);
  base::CommandLine* line = base::CommandLine::ForCurrentProcess();

  logging::LoggingSettings settings;
  settings.logging_dest = logging::LOG_TO_SYSTEM_DEBUG_LOG;
  CHECK(logging::InitLogging(settings));

  if (line->HasSwitch("h") || line->HasSwitch("help")) {
    const char* help_str =
        "Usage: quic_server [options]\n"
        "\n"
        "Options:\n"
        "-h, --help                  show this help message and exit\n"
        "--port=<port>               specify the port to listen on\n"
        "--mode=<cache|proxy>        Specify mode of operation: Proxy will "
        "serve response from\n"
        "                            a backend server and Cache will serve it "
        "from a cache dir\n"
        "--quic_response_cache_dir  directory containing response data\n"
        "                            to load\n"
        "--quic_proxy_backend_url=<http/https>://<hostname_ip>:<port_number> \n"
        "                            The URL for the single backend server "
        "hostname \n"
        "                            For example, \"http://xyz.com:80\"\n"
        "--certificate_file=<file>   path to the certificate chain\n"
        "--key_file=<file>           path to the pkcs8 private key\n";
    std::cout << help_str;
    exit(0);
  }

  // Serve from local cache or function as a reverse proxy
  net::QuicHttpResponseCache response_cache;
  net::QuicHttpResponseProxy quic_proxy_context;
  if (line->HasSwitch("mode")) {
    FLAGS_quic_mode = line->GetSwitchValueASCII("mode");
  }
  if (FLAGS_quic_mode.compare("cache") == 0) {
    if (line->HasSwitch("quic_response_cache_dir")) {
      response_cache.InitializeFromDirectory(
          line->GetSwitchValueASCII("quic_response_cache_dir"));
    }
  } else if (FLAGS_quic_mode.compare("proxy") == 0) {
    if (line->HasSwitch("quic_proxy_backend_url")) {
      FLAGS_quic_proxy_backend_url =
          line->GetSwitchValueASCII("quic_proxy_backend_url");
      if (quic_proxy_context.Initialize(FLAGS_quic_proxy_backend_url) != true) {
        LOG(ERROR) << "--quic_proxy_backend_url "
                   << FLAGS_quic_proxy_backend_url << " is not valid !";
        return 1;
      }
    }
  } else {
    LOG(ERROR) << "unknown --mode. cache or proxy are valid modes of operation";
    return 1;
  }

  if (line->HasSwitch("port")) {
    if (!base::StringToInt(line->GetSwitchValueASCII("port"), &FLAGS_port)) {
      LOG(ERROR) << "--port must be an integer\n";
      return 1;
    }
  }

  if (!line->HasSwitch("certificate_file")) {
    LOG(ERROR) << "missing --certificate_file";
    return 1;
  }

  if (!line->HasSwitch("key_file")) {
    LOG(ERROR) << "missing --key_file";
    return 1;
  }

  net::IPAddress ip = net::IPAddress::IPv6AllZeros();

  net::QuicConfig config;
  net::QuicSimpleServer server(
      CreateProofSource(line->GetSwitchValuePath("certificate_file"),
                        line->GetSwitchValuePath("key_file")),
      config, net::QuicCryptoServerConfig::ConfigOptions(),
      net::AllSupportedVersions(), &response_cache, &quic_proxy_context);

  int rc = server.Listen(net::IPEndPoint(ip, FLAGS_port));
  if (rc < 0) {
    return 1;
  }

  base::RunLoop().Run();

  return 0;
}
