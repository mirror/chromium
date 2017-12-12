// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdio.h>
#include <iostream>
#include <set>

#include "base/command_line.h"
#include "base/strings/string_piece.h"

#include "wpkg/webpackage_reader.h"

using namespace wpkg;

class TestWebPackageReaderClient : public WebPackageReaderClient {
 public:
  std::map<int, std::string> request_map_;
  std::set<int> text_response_;

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
    request_map_.insert(
        std::make_pair(request_id, request_headers.at(":path")));
  }
  void OnResourceResponse(int request_id,
                          const HttpHeaders& response_headers) override {
    LOG(INFO) << "=== OnResourceResponse ===";
    LOG(INFO) << "* request id: " << request_id;
    LOG(INFO) << "* response:";
    for (const auto& kv : response_headers) {
      LOG(INFO) << "  - " << kv.first << " => " << kv.second;
      //      if (kv.first == "content-type" && kv.second.find("text/") == 0)
      //        text_response_.insert(request_id);
    }
  }

  void OnDataReceived(int request_id, const void* data, size_t size) override {
    LOG(INFO) << "=== OnDataReceived ===";
    LOG(INFO) << "* request id: " << request_id;
    LOG(INFO) << "* response chunk length: " << size;
    /*
    if (text_response_.find(request_id) != text_response_.end()) {
      LOG(INFO) << "## " << request_map_[request_id];
      LOG(INFO) << base::StringPiece(static_cast<const char*>(data),
    size).as_string(); text_response_.erase(request_id);
    }
    */
  }

  void OnNotifyFinished(int request_id) override {
    LOG(INFO) << "=== OnNofityFinished ===";
    LOG(INFO) << "* request id: " << request_id;
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
