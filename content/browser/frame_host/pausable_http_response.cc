// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/frame_host/pausable_http_response.h"
#include "content/public/browser/browser_thread.h"

namespace content {

PausableHttpResponse::PausableHttpResponse(
    PausableHttpResponseController* controller)
    : controller_(controller), weak_ptr_factory_(this) {}
PausableHttpResponse::~PausableHttpResponse() {}

// Called on the embedded test server IO thread.
void PausableHttpResponse::SendResponse(
    const net::test_server::SendBytesCallback& send,
    const net::test_server::SendCompleteCallback& done) {
  // Send response's header, then rely on the controller for waiting and sending
  // the response's body.
  send.Run(BuildHeader(),
           base::Bind(&PausableHttpResponseController::OnHeaderSentIO,
                      base::Unretained(controller_), content(),
                      weak_ptr_factory_.GetWeakPtr(), send, done));
}

PausableHttpResponseController::PausableHttpResponseController() {}
PausableHttpResponseController::~PausableHttpResponseController() {
  DCHECK(resume_called_) << "Resume() must be called at one point.";
}

void PausableHttpResponseController::WaitUntilHeaderIsSent() {
  if (!resume_) {
    wait_header_sent_loop_.reset(new base::RunLoop);
    wait_header_sent_loop_->Run();
    DCHECK(resume_);
  }
}

void PausableHttpResponseController::Resume() {
  DCHECK(resume_) << "Headers haven't been sent yet, please use "
                     "WaitUntilHeaderIsSent() before Resume()";
  resume_.Run();
  resume_called_ = true;
}

// Called on the embedded test server IO thread.
void PausableHttpResponseController::OnHeaderSentIO(
    std::string content,
    base::WeakPtr<PausableHttpResponse> response,
    const net::test_server::SendBytesCallback& send,
    const net::test_server::SendCompleteCallback& done) {
  // Wait until |Resume| is called.
  base::RunLoop loop;
  BrowserThread::PostTask(
      BrowserThread::UI, FROM_HERE,
      base::BindOnce(&PausableHttpResponseController::OnHeaderSentUI,
                     base::Unretained(this), loop.QuitClosure()));
  loop.Run();

  // TODO(arthursonzogni): Is it useful? Can the response be detroyed while the
  // previous run loop run?
  DCHECK(response) << "The response has been destroyed while running the loop";

  // Send the response's body.
  send.Run(content, done);
};

void PausableHttpResponseController::OnHeaderSentUI(base::Closure resume) {
  resume_ = std::move(resume);
  if (wait_header_sent_loop_)
    wait_header_sent_loop_->Quit();
};

}  // namespace content
