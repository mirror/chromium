// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cronet_c.h"

#include "base/strings/stringprintf.h"
#include "components/cronet/native/test_util.h"
#include "components/grpc_support/test/get_stream_engine.h"
#include "net/base/net_errors.h"
#include "net/cert/mock_cert_verifier.h"

namespace grpc_support {

Cronet_EnginePtr g_cronet_engine = nullptr;

stream_engine* GetTestStreamEngine(int port) {
  return Cronet_Engine_GetStreamEngine(g_cronet_engine);
}

void StartTestStreamEngine(int port) {
  CHECK(!g_cronet_engine);
  g_cronet_engine = cronet::test::CreateTestEngine(port);
}

void ShutdownTestStreamEngine() {
  Cronet_Engine_Destroy(g_cronet_engine);
  g_cronet_engine = nullptr;
}

}  // namespace grpc_support
