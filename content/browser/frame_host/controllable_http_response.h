// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/public/test/content_browser_test.h"
#include "content/public/test/content_browser_test_utils.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "net/test/embedded_test_server/http_request.h"
#include "net/test/embedded_test_server/http_response.h"

namespace content {

// A response whose actions are triggered from the UI thread.
class ControllableHttpResponse : public net::test_server::HttpResponse {
 public:
  class Controller;
  friend class Controller;

  ControllableHttpResponse(Controller* controller);
  ~ControllableHttpResponse() override;

 private:
  void SendResponse(
      const net::test_server::SendBytesCallback& send,
      const net::test_server::SendCompleteCallback& done) override;

  Controller* controller_;
};

// The controller of the ControllableHttpResponse. Used to resume a response.
// It can also wait for the header to be sent.
class ControllableHttpResponse::Controller {
  friend class ControllableHttpResponse;

 public:
  Controller();
  ~Controller();

  // Helper function.
  // It is meant to be used in EmbeddedTestServer::RegisterRequestHandler().
  // For a request with a given |relative_url|, it will return a new
  // ControllableHttpResponse associated with this.
  net::test_server::EmbeddedTestServer::HandleRequestCallback HandleRequest(
      const std::string& relative_url);

  // These method are intented to be used in order.
  // 1) Wait for the response to be requested by the browser.
  void WaitConnection();

  // 2) Send data to the browser. May be called several time.
  void Send(const std::string& bytes);

  // 3) Notify there are no more data to be sent.
  void Done();

 private:
  void OnConnectionOpened(scoped_refptr<base::SingleThreadTaskRunner>
                              embedded_test_server_task_runner,
                          net::test_server::SendBytesCallback send,
                          net::test_server::SendCompleteCallback done);

  scoped_refptr<base::SingleThreadTaskRunner> embedded_test_server_task_runner_;
  net::test_server::SendBytesCallback send_;
  net::test_server::SendCompleteCallback done_;
  base::RunLoop loop_;
};

}  // namespace content
