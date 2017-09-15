// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "content/browser/frame_host/controllable_http_response.h"

#include "base/threading/thread_task_runner_handle.h"
#include "content/public/browser/browser_thread.h"

namespace content {

ControllableHttpResponse::ControllableHttpResponse(Controller* controller)
    : controller_(controller) {}
ControllableHttpResponse::~ControllableHttpResponse() {}

ControllableHttpResponse::Controller::Controller() {}
ControllableHttpResponse::Controller::~Controller() {}

namespace {
// Helper function used in CreateRequestHandler below
std::unique_ptr<net::test_server::HttpResponse> RequestHandler(
    const std::string& relative_url,
    ControllableHttpResponse::Controller* controller,
    const net::test_server::HttpRequest& request) {
  if (request.relative_url == relative_url)
    return std::make_unique<ControllableHttpResponse>(controller);

  return nullptr;
}

}  // namespace

net::test_server::EmbeddedTestServer::HandleRequestCallback
ControllableHttpResponse::Controller::HandleRequest(
    const std::string& relative_url) {
  return base::Bind(&RequestHandler, relative_url, this);
}

void ControllableHttpResponse::SendResponse(
    const net::test_server::SendBytesCallback& send,
    const net::test_server::SendCompleteCallback& done) {
  BrowserThread::PostTask(
      BrowserThread::UI, FROM_HERE,
      base::Bind(&Controller::OnConnectionOpened, base::Unretained(controller_),
                 base::ThreadTaskRunnerHandle::Get(), send, done));
}

void ControllableHttpResponse::Controller::WaitConnection() {
  if (!embedded_test_server_task_runner_) {
    loop_.Run();
    DCHECK(embedded_test_server_task_runner_);
  }
}

void ControllableHttpResponse::Controller::OnConnectionOpened(
    scoped_refptr<base::SingleThreadTaskRunner>
        embedded_test_server_task_runner,
    net::test_server::SendBytesCallback send,
    net::test_server::SendCompleteCallback done) {
  embedded_test_server_task_runner_ = embedded_test_server_task_runner;
  send_ = send;
  done_ = done;
  if (loop_.running())
    loop_.Quit();
}

void ControllableHttpResponse::Controller::Send(const std::string& bytes) {
  DCHECK(embedded_test_server_task_runner_) << "Call WaitReady() before Send()";
  base::RunLoop loop;
  embedded_test_server_task_runner_->PostTask(
      FROM_HERE, base::BindOnce(send_, bytes, loop.QuitClosure()));
  loop.Run();
}

void ControllableHttpResponse::Controller::Done() {
  DCHECK(embedded_test_server_task_runner_) << "Call WaitReady() before Done()";
  embedded_test_server_task_runner_->PostTask(FROM_HERE, done_);
}

}  // namespace content
