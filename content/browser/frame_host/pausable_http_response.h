// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/public/test/content_browser_test.h"
#include "content/public/test/content_browser_test_utils.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "net/test/embedded_test_server/http_request.h"
#include "net/test/embedded_test_server/http_response.h"

namespace content {

class PausableHttpResponseController;

// A response that will:
// 1) Send the header.
// 2) Wait for its controller to call Resume().
// 3) Send the response's body.
class PausableHttpResponse : public net::test_server::BasicHttpResponse {
 public:
  PausableHttpResponse(PausableHttpResponseController* controller);
  ~PausableHttpResponse() override;

  void SendResponse(
      const net::test_server::SendBytesCallback& send,
      const net::test_server::SendCompleteCallback& done) override;

 private:
  PausableHttpResponseController* controller_;
  base::WeakPtrFactory<PausableHttpResponse> weak_ptr_factory_;
};

// The controller of the PausableHttpResponse. Used to resume a response. It can
// also wait for the header to be sent.
class PausableHttpResponseController {
  friend class PausableHttpResponse;

 public:
  PausableHttpResponseController();
  ~PausableHttpResponseController();

  void WaitUntilHeaderIsSent();
  void Resume();

 private:
  // Called on the embedded test server IO thread.
  void OnHeaderSentIO(std::string content,
                      base::WeakPtr<PausableHttpResponse> response,
                      const net::test_server::SendBytesCallback& send,
                      const net::test_server::SendCompleteCallback& done);

  // Called on the UI thread. The response is paused and |resume| can be used to
  // resume it.
  void OnHeaderSentUI(base::Closure resume);

  // UI thread objects:
  std::unique_ptr<base::RunLoop> wait_header_sent_loop_;
  base::Closure resume_;
  bool resume_called_ = false;
};
}  // namespace content
