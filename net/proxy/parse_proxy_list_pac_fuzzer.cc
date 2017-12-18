// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stddef.h>
#include <stdint.h>

#include "net/proxy/proxy_list.h"
#include "net/traffic_annotation/network_traffic_annotation_test_helper.h"

// Entry point for LibFuzzer.
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  net::ProxyList list;
  std::string input(data, data + size);
  list.SetFromPacString(input, PARTIAL_TRAFFIC_ANNOTATION_FOR_TESTS);
  return 0;
}
