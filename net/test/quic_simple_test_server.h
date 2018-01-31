// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_TEST_QUIC_SIMPLE_TEST_SERVER_H_
#define NET_TEST_QUIC_SIMPLE_TEST_SERVER_H_

#include <string>

namespace net {

class QuicSimpleTestServer {
 public:
  static const std::string domain();
  static const std::string host();
  static const std::string url();

  static const std::string status_header_name();

  // Hello Url returns response with HTTP/2 headers and trailers.
  static const std::string hello_path();
  static const std::string hello_body_value();
  static const std::string hello_status();

  static const std::string hello_header_name();
  static const std::string hello_header_value();

  static const std::string hello_trailer_name();
  static const std::string hello_trailer_value();

  // Simple Url returns response without HTTP/2 trailers.
  static const std::string simple_url();
  static const std::string simple_body_value();
  static const std::string simple_status();
  static const std::string simple_header_name();
  static const std::string simple_header_value();

  static bool Start();

  static void Shutdown();

  // Shuts down the server dispatcher, which results in sending ConnectionClose
  // frames to all connected clients.
  static void ShutdownDispatcher();

  static int GetPort();
};

}  // namespace net

#endif  // NET_TEST_QUIC_SIMPLE_TEST_SERVER_H_
