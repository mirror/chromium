// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_TEST_QUIC_SIMPLE_TEST_SERVER_H_
#define NET_TEST_QUIC_SIMPLE_TEST_SERVER_H_

namespace net {

class QuicSimpleTestServer {
 public:
  static const char kTestServerDomain[];
  static const char kTestServerHost[];
  static const char kTestServerUrl[];

  static const char kStatusHeader[];

  static const char kHelloPath[];
  static const char kHelloBodyValue[];
  static const char kHelloStatus[];

  static const char kHelloHeaderName[];
  static const char kHelloHeaderValue[];

  static const char kHelloTrailerName[];
  static const char kHelloTrailerValue[];

  // Simple Url returns response without HTTP/2 trailers.
  static const char kTestServerSimpleUrl[];
  static const char kSimpleBodyValue[];
  static const char kSimpleStatus[];
  static const char kSimpleHeaderName[];
  static const char kSimpleHeaderValue[];

  static bool Start();

  static void Shutdown();

  // Shuts down the server dispatcher, which results in sending ConnectionClose
  // frames to all connected clients.
  static void ShutdownDispatcher();

  static int GetPort();
};

}  // namespace net

#endif  // NET_TEST_QUIC_SIMPLE_TEST_SERVER_H_
