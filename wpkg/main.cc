// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdio.h>
#include <iostream>

#include "base/command_line.h"

#include "wpkg/webpackage_reader.h"

using namespace wpkg;

class TestWebPackageReaderClient : public WebPackageReaderClient {
 public:
  TestWebPackageReaderClient() = default;

  void OnOrigin(const std::string& origin) override {
    LOG(INFO) << "=== Origin: " << origin;
  }
  void OnResourceRequest(const HttpHeaders& request_headers,
                         int request_id) override {
    LOG(INFO) << "=== OnResourceRequest ===";
    LOG(INFO) << "* request id: " << request_id;
    LOG(INFO) << "* request :method=" << request_headers.at(":method")
              << " :scheme=" << request_headers.at(":scheme")
              << " :authority=" << request_headers.at(":authority")
              << " :path=" << request_headers.at(":path");
  }
  void OnResourceResponse(int request_id,
                          const HttpHeaders& response_headers,
                          const void* data,
                          size_t size) override {
    LOG(INFO) << "=== OnResourceResponse ===";
    LOG(INFO) << "* request id: " << request_id;
    LOG(INFO) << "* response:";
    for (const auto& kv : response_headers) {
      LOG(INFO) << "  - " << kv.first << " => " << kv.second;
    }
    LOG(INFO) << "* response length: " << size;
  }
  void OnEnd() override { LOG(INFO) << ("==END"); }
};

int main(int argc, char* argv[]) {
  base::CommandLine::Init(argc, argv);
  base::CommandLine* cmd = base::CommandLine::ForCurrentProcess();
  base::CommandLine::StringVector args = cmd->GetArgs();
  if (args.size() != 1) {
    std::cerr << argv[0] << " [wpk file]" << std::endl;
    return EXIT_FAILURE;
  }

  TestWebPackageReaderClient client;
  WebPackageReader reader(&client);

  FILE* fp = fopen(args[0].c_str(), "r");
  for (;;) {
    uint8_t buf[4096];
    size_t nread = fread(buf, 1, sizeof(buf), fp);
    if (nread == 0)
      break;
    reader.ConsumeDataChunk(buf, nread);
  }
  reader.ConsumeEOS();
  fclose(fp);
  return 0;
}
