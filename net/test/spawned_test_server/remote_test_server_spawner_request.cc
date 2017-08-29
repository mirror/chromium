// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/test/spawned_test_server/remote_test_server_spawner_request.h"

#include <utility>

#include "base/location.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/single_thread_task_runner.h"
#include "base/synchronization/waitable_event.h"
#include "base/test/test_timeouts.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "build/build_config.h"
#include "net/base/elements_upload_data_stream.h"
#include "net/base/io_buffer.h"
#include "net/base/port_util.h"
#include "net/base/upload_bytes_element_reader.h"
#include "net/http/http_response_headers.h"
#include "net/url_request/url_request.h"
#include "net/url_request/url_request_test_util.h"
#include "url/gurl.h"

namespace net {

static const int kBufferSize = 2048;

// Implemenation of RemoteTestServerSpawnerRequest that runs on background IO
// thread.
class RemoteTestServerSpawnerRequest::Core : public URLRequest::Delegate {
 public:
  Core();
  ~Core() override;

  void SendRequest(const GURL& url, const std::string& post_data);
  bool WaitResult(std::string* response) WARN_UNUSED_RESULT;

 private:
  // URLRequest::Delegate methods.
  void OnResponseStarted(URLRequest* request, int net_error) override;
  void OnReadCompleted(URLRequest* request, int num_bytes) override;

  // Reads Result from the response.
  void ReadResult(URLRequest* request);

  // Called upon completion of the spawner command.
  void OnCommandCompleted(URLRequest* request, int net_error);

  // Timeout timer task.
  void OnTimeout();

  void FinishCurrentRequestOnIOThread();

  // Request results.
  int result_code_ = 0;
  std::string data_received_;

  // WaitableEvent to notify whether the communication is done.
  base::WaitableEvent event_;

  // Helper to add spawner port to the list of the globally explicitly allowed
  // ports.
  std::unique_ptr<ScopedPortException> allowed_port_;

  // Request context used by |request_|.
  std::unique_ptr<URLRequestContext> context_;

  std::unique_ptr<URLRequest> request_;

  scoped_refptr<IOBuffer> read_buffer_;

  std::unique_ptr<base::OneShotTimer> timeout_timer_;

  THREAD_CHECKER(thread_checker_);

  DISALLOW_COPY_AND_ASSIGN(Core);
};

RemoteTestServerSpawnerRequest::Core::Core()
    : event_(base::WaitableEvent::ResetPolicy::AUTOMATIC,
             base::WaitableEvent::InitialState::NOT_SIGNALED),
      read_buffer_(new IOBuffer(kBufferSize)) {
  DETACH_FROM_THREAD(thread_checker_);
}

void RemoteTestServerSpawnerRequest::Core::SendRequest(
    const GURL& url,
    const std::string& post_data) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

  // Prepare the URLRequest for sending the command.
  DCHECK(!request_.get());
  context_.reset(new TestURLRequestContext);
  allowed_port_ = std::make_unique<ScopedPortException>(url.EffectiveIntPort());
  request_ = context_->CreateRequest(url, DEFAULT_PRIORITY, this);

  DCHECK(request_);

  if (post_data.empty()) {
    request_->set_method("GET");
  } else {
    request_->set_method("POST");
    std::unique_ptr<UploadElementReader> reader(
        UploadOwnedBytesElementReader::CreateWithString(post_data));
    request_->set_upload(
        ElementsUploadDataStream::CreateWithReader(std::move(reader), 0));
    HttpRequestHeaders headers;
    headers.SetHeader(HttpRequestHeaders::kContentType, "application/json");
    request_->SetExtraRequestHeaders(headers);
  }

  timeout_timer_ = std::make_unique<base::OneShotTimer>();
  timeout_timer_->Start(FROM_HERE, TestTimeouts::action_max_timeout(),
                        base::Bind(&Core::OnTimeout, base::Unretained(this)));

  request_->Start();
}

RemoteTestServerSpawnerRequest::Core::~Core() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
}

bool RemoteTestServerSpawnerRequest::Core::WaitResult(std::string* response) {
  event_.Wait();

  if (response)
    *response = data_received_;

  return result_code_ == OK;
}

void RemoteTestServerSpawnerRequest::Core::OnTimeout() {
  DETACH_FROM_THREAD(thread_checker_);

  // Set the result code and cancel the timed-out task.
  int result = request_->CancelWithError(ERR_TIMED_OUT);
  timeout_timer_.reset();

  OnCommandCompleted(request_.get(), result);
}

void RemoteTestServerSpawnerRequest::Core::OnCommandCompleted(
    URLRequest* request,
    int net_error) {
  DETACH_FROM_THREAD(thread_checker_);
  DCHECK_NE(ERR_IO_PENDING, net_error);

  DCHECK_EQ(request, request_.get());

  // If request has failed, return the error code.
  if (net_error != OK) {
    LOG(ERROR) << "request failed, error: " << ErrorToString(net_error);
    result_code_ = net_error;
  } else if (request->GetResponseCode() != 200) {
    LOG(ERROR) << "Spawner server returned bad status: "
               << request->response_headers()->GetStatusLine() << ", "
               << data_received_;
    result_code_ = ERR_FAILED;
  }

  if (result_code_ != OK)
    data_received_.clear();

  request_.reset();
  context_.reset();

  DCHECK(!event_.IsSignaled());
  event_.Signal();
}

void RemoteTestServerSpawnerRequest::Core::ReadResult(URLRequest* request) {
  DETACH_FROM_THREAD(thread_checker_);
  DCHECK_EQ(request, request_.get());

  // Read as many bytes as are available synchronously.
  while (true) {
    int rv = request->Read(read_buffer_.get(), kBufferSize);
    if (rv == ERR_IO_PENDING)
      return;

    if (rv <= 0) {
      OnCommandCompleted(request, rv);
      return;
    }

    data_received_.append(read_buffer_->data(), rv);
  }
}

void RemoteTestServerSpawnerRequest::Core::OnResponseStarted(
    URLRequest* request,
    int net_error) {
  DETACH_FROM_THREAD(thread_checker_);
  DCHECK_EQ(request, request_.get());
  DCHECK_NE(ERR_IO_PENDING, net_error);

  if (net_error != OK) {
    OnCommandCompleted(request, net_error);
    return;
  }

  ReadResult(request);
}

void RemoteTestServerSpawnerRequest::Core::OnReadCompleted(URLRequest* request,
                                                           int read_result) {
  DETACH_FROM_THREAD(thread_checker_);
  DCHECK_NE(ERR_IO_PENDING, read_result);

  if (!request_.get())
    return;
  DCHECK_EQ(request, request_.get());

  if (read_result > 0) {
    data_received_.append(read_buffer_->data(), read_result);

    // Keep reading.
    ReadResult(request);
  } else {
    // |bytes_read| < 0
    int net_error = read_result;
    OnCommandCompleted(request, net_error);
  }
}

RemoteTestServerSpawnerRequest::RemoteTestServerSpawnerRequest(
    scoped_refptr<base::SingleThreadTaskRunner> io_task_runner,
    const GURL& url,
    const std::string& post_data)
    : io_task_runner_(io_task_runner), core_(new Core()) {
  io_task_runner_->PostTask(
      FROM_HERE, base::BindOnce(&Core::SendRequest,
                                base::Unretained(core_.get()), url, post_data));
}

RemoteTestServerSpawnerRequest::~RemoteTestServerSpawnerRequest() {
  DETACH_FROM_THREAD(thread_checker_);
  io_task_runner_->DeleteSoon(FROM_HERE, core_.release());
}

bool RemoteTestServerSpawnerRequest::WaitResult(std::string* response) {
  DETACH_FROM_THREAD(thread_checker_);
  return core_->WaitResult(response);
}

}  // namespace net
