// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PPAPI_TESTS_TEST_HOST_RESOLVER_H_
#define PPAPI_TESTS_TEST_HOST_RESOLVER_H_

#include <string>

#include "ppapi/c/dev/ppb_host_resolver_dev.h"
#include "ppapi/tests/test_case.h"

namespace pp {
class HostResolver_Dev;
class NetAddress;
class TCPSocket_Dev;
}  // namespace pp

class TestHostResolver : public TestCase {
 public:
  explicit TestHostResolver(TestingInstance* instance);

  // TestCase implementation.
  virtual bool Init();
  virtual void RunTests(const std::string& filter);

 private:
  std::string SyncConnect(pp::TCPSocket_Dev* socket,
                          const pp::NetAddress& address);
  std::string SyncRead(pp::TCPSocket_Dev* socket,
                       char* buffer,
                       int32_t num_bytes,
                       int32_t* bytes_read);
  std::string SyncWrite(pp::TCPSocket_Dev* socket,
                        const char* buffer,
                        int32_t num_bytes,
                        int32_t* bytes_written);
  std::string CheckHTTPResponse(pp::TCPSocket_Dev* socket,
                                const std::string& request,
                                const std::string& response);
  std::string SyncResolve(pp::HostResolver_Dev* host_resolver,
                          const std::string& host,
                          uint16_t port,
                          const PP_HostResolver_Hint_Dev& hint);
  std::string ParameterizedTestResolve(const PP_HostResolver_Hint_Dev& hint);

  std::string TestEmpty();
  std::string TestResolve();
  std::string TestResolveIPv4();

  std::string host_;
  uint16_t port_;
};

#endif  // PPAPI_TESTS_TEST_HOST_RESOLVER_H_
