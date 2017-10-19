// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/safe_browsing/db/v4_get_hash_protocol_manager.h"

#include <string>

#include "components/safe_browsing/db/safebrowsing.pb.h"

class V4GetHashProtocolManagerFuzzer {
 public:
  static int FuzzMetadata(const uint8_t* data, size_t size) {
    safe_browsing::FindFullHashesResponse response;
    std::string input(reinterpret_cast<const char*>(data), size);
    if (!response.ParseFromString(input))
      return 0;
    safe_browsing::ThreatMetadata metadata;
    for (const safe_browsing::ThreatMatch& match : response.matches()) {
      safe_browsing::V4GetHashProtocolManager::ParseMetadata(match, &metadata);
    }
    return 0;
  }
};

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  return V4GetHashProtocolManagerFuzzer::FuzzMetadata(data, size);
}
