// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_TEST_CONTROLLABLE_HTTP_RESPONSE_H_
#define CONTENT_PUBLIC_TEST_CONTROLLABLE_HTTP_RESPONSE_H_

#include "net/test/embedded_test_server/embedded_test_server.h"
#include "net/test/embedded_test_server/http_request.h"
#include "net/test/embedded_test_server/http_response.h"

namespace content {

// A response that can be controlled on the UI thread.
// It is used for waiting for a connection, sending data and closing it.
// This class can only handle one request at a time. No new request should be
// issued to the same URL while a previous one has not been served yet.
class ControllableHttpResponse {
 public:
  ControllableHttpResponse(
      net::test_server::EmbeddedTestServer* embedded_test_server,
      const std::string& relative_path);
  virtual ~ControllableHttpResponse();

  // These method are intented to be used in order.
  // 1) Wait for the response to be requested by the browser.
  void WaitRequest();

  // 2) Send data to the browser. May be called several time.
  void Send(const std::string& bytes);

  // 3) Notify there are no more data to be sent.
  void Done();

 private:
  class Interceptor;

  void OnRequest(scoped_refptr<base::SingleThreadTaskRunner>
                     embedded_test_server_task_runner,
                 net::test_server::SendBytesCallback send,
                 net::test_server::SendCompleteCallback done);

  static std::unique_ptr<net::test_server::HttpResponse> RequestHandler(
      const std::string& relative_url,
      base::WeakPtr<ControllableHttpResponse> controller,
      const net::test_server::HttpRequest& request);

  bool ready_to_send_data_ = false;
  std::unique_ptr<base::RunLoop> loop_;
  scoped_refptr<base::SingleThreadTaskRunner> embedded_test_server_task_runner_;
  net::test_server::SendBytesCallback send_;
  net::test_server::SendCompleteCallback done_;
  base::WeakPtrFactory<ControllableHttpResponse> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(ControllableHttpResponse);
};

}  // namespace content

#endif  //  CONTENT_PUBLIC_TEST_CONTROLLABLE_HTTP_RESPONSE_H_
