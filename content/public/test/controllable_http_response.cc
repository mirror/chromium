// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/public/test/controllable_http_response.h"

#include "base/logging.h"
#include "base/threading/thread_task_runner_handle.h"
#include "content/public/browser/browser_thread.h"

namespace content {

class ControllableHttpResponse::Interceptor
    : public net::test_server::HttpResponse {
 public:
  explicit Interceptor(base::WeakPtr<ControllableHttpResponse> controller)
      : controller_(controller) {}
  ~Interceptor() override {}

 private:
  void SendResponse(
      const net::test_server::SendBytesCallback& send,
      const net::test_server::SendCompleteCallback& done) override {
    BrowserThread::PostTask(
        BrowserThread::UI, FROM_HERE,
        base::BindOnce(&ControllableHttpResponse::OnRequest, controller_,
                       base::ThreadTaskRunnerHandle::Get(), send, done));
  }

  base::WeakPtr<ControllableHttpResponse> controller_;

  DISALLOW_COPY_AND_ASSIGN(Interceptor);
};

ControllableHttpResponse::ControllableHttpResponse(
    net::test_server::EmbeddedTestServer* embedded_test_server,
    const std::string& relative_url)
    : loop_(new base::RunLoop), weak_ptr_factory_(this) {
  DCHECK(!embedded_test_server->Started()) << "ControllableHttpResponse must "
                                              "be instanciated before starting "
                                              "the EmbeddedTestServer";
  embedded_test_server->RegisterRequestHandler(
      base::Bind(&ControllableHttpResponse::RequestHandler, relative_url,
                 weak_ptr_factory_.GetWeakPtr()));
}

ControllableHttpResponse::~ControllableHttpResponse() {}

void ControllableHttpResponse::WaitRequest() {
  DCHECK(!ready_to_send_data_);
  loop_->Run();
  DCHECK(embedded_test_server_task_runner_);
  ready_to_send_data_ = true;
}

void ControllableHttpResponse::Send(const std::string& bytes) {
  DCHECK(ready_to_send_data_) << "Send() called without any opened connection. "
                                 "Did you call WaitRequest()?";
  base::RunLoop loop;
  embedded_test_server_task_runner_->PostTask(
      FROM_HERE, base::BindOnce(send_, bytes, loop.QuitClosure()));
  loop.Run();
}

void ControllableHttpResponse::Done() {
  DCHECK(ready_to_send_data_) << "Done() called without any opened connection. "
                                 "Did you call WaitRequest()?";
  embedded_test_server_task_runner_->PostTask(FROM_HERE, done_);

  // Reset the instance in order to make it reusable.
  ready_to_send_data_ = false;
  loop_.reset(new base::RunLoop);
  send_ = net::test_server::SendBytesCallback();
  done_ = net::test_server::SendCompleteCallback();
  embedded_test_server_task_runner_ = nullptr;
}

void ControllableHttpResponse::OnRequest(
    scoped_refptr<base::SingleThreadTaskRunner>
        embedded_test_server_task_runner,
    net::test_server::SendBytesCallback send,
    net::test_server::SendCompleteCallback done) {
  DCHECK(!embedded_test_server_task_runner_)
      << "A ControllableHttpResponse can only handle one request at a time";
  embedded_test_server_task_runner_ = embedded_test_server_task_runner;
  send_ = send;
  done_ = done;
  loop_->Quit();
}

// Helper function used in the ControllableHttpResponse constructor.
// static
std::unique_ptr<net::test_server::HttpResponse>
ControllableHttpResponse::RequestHandler(
    const std::string& relative_url,
    base::WeakPtr<ControllableHttpResponse> controller,
    const net::test_server::HttpRequest& request) {
  if (request.relative_url == relative_url)
    return std::make_unique<ControllableHttpResponse::Interceptor>(controller);

  return nullptr;
}

}  // namespace content
