// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/cronet/native/test_url_request_callback.h"

namespace {}  // namespace

namespace cronet {
namespace test {

TestUrlRequestCallback::TestUrlRequestCallback()
    : done_(base::WaitableEvent::ResetPolicy::MANUAL,
            base::WaitableEvent::InitialState::NOT_SIGNALED),
      step_block_(base::WaitableEvent::ResetPolicy::MANUAL,
                  base::WaitableEvent::InitialState::NOT_SIGNALED) {}

TestUrlRequestCallback::~TestUrlRequestCallback() {}

Cronet_UrlRequestCallbackPtr
TestUrlRequestCallback::CreateUrlRequestCallback() {
  Cronet_UrlRequestCallbackPtr callback = Cronet_UrlRequestCallback_CreateStub(
      TestUrlRequestCallback::OnRedirectReceived,
      TestUrlRequestCallback::OnResponseStarted,
      TestUrlRequestCallback::OnReadCompleted,
      TestUrlRequestCallback::OnSucceeded, TestUrlRequestCallback::OnFailed,
      TestUrlRequestCallback::OnCanceled);
  Cronet_UrlRequestCallback_SetContext(callback, this);
  return callback;
}

void TestUrlRequestCallback::OnRedirectReceived(Cronet_UrlRequestPtr request,
                                                Cronet_UrlResponseInfoPtr info,
                                                CharString newLocationUrl) {
  CheckExecutorThread();

  CHECK(!Cronet_UrlRequest_IsDone(request));
  CHECK(response_step_ == NOTHING || response_step_ == ON_RECEIVED_REDIRECT);
  CHECK(!last_error_);

  response_step_ = ON_RECEIVED_REDIRECT;
  redirect_url_list_.push_back(newLocationUrl);
  ++redirect_count_;
  if (MaybeCancelOrPause(request)) {
    return;
  }
  Cronet_UrlRequest_FollowRedirect(request);
}

void TestUrlRequestCallback::OnResponseStarted(Cronet_UrlRequestPtr request,
                                               Cronet_UrlResponseInfoPtr info) {
  CheckExecutorThread();
  CHECK(!Cronet_UrlRequest_IsDone(request));
  CHECK(response_step_ == NOTHING || response_step_ == ON_RECEIVED_REDIRECT);
  CHECK(!last_error_);
  response_step_ = ON_RESPONSE_STARTED;
  last_response_info_ = info;
  if (MaybeCancelOrPause(request)) {
    return;
  }
  StartNextRead(request);
}

void TestUrlRequestCallback::OnReadCompleted(Cronet_UrlRequestPtr request,
                                             Cronet_UrlResponseInfoPtr info,
                                             Cronet_BufferPtr buffer,
                                             uint64_t bytes_read) {
  CheckExecutorThread();
  CHECK(!Cronet_UrlRequest_IsDone(request));
  CHECK(response_step_ == ON_RESPONSE_STARTED ||
        response_step_ == ON_READ_COMPLETED);
  CHECK(!last_error_);
  response_step_ = ON_READ_COMPLETED;
  last_response_info_ = info;
  response_data_length_ += bytes_read;

  if (accumulate_response_data_) {
    std::string last_read_data(
        reinterpret_cast<char*>(Cronet_Buffer_GetData(buffer)), bytes_read);
    response_as_string_ += last_read_data;
  }

  if (MaybeCancelOrPause(request)) {
    return;
  }
  StartNextRead(request, buffer);
}

void TestUrlRequestCallback::OnSucceeded(Cronet_UrlRequestPtr request,
                                         Cronet_UrlResponseInfoPtr info) {
  CheckExecutorThread();
  CHECK(Cronet_UrlRequest_IsDone(request));
  CHECK(response_step_ == ON_RESPONSE_STARTED ||
        response_step_ == ON_READ_COMPLETED);
  CHECK(!on_error_called_);
  CHECK(!on_canceled_called_);
  CHECK(!last_error_);
  response_step_ = ON_SUCCEEDED;
  last_response_info_ = info;

  SignalDone();
  MaybeCancelOrPause(request);
}

void TestUrlRequestCallback::OnFailed(Cronet_UrlRequestPtr request,
                                      Cronet_UrlResponseInfoPtr info,
                                      Cronet_ErrorPtr error) {
  CheckExecutorThread();
  CHECK(Cronet_UrlRequest_IsDone(request));
  // Shouldn't happen after success.
  CHECK(response_step_ != ON_SUCCEEDED);
  // Should happen at most once for a single request.
  CHECK(!on_error_called_);
  CHECK(!on_canceled_called_);
  CHECK(!last_error_);

  response_step_ = ON_FAILED;
  on_error_called_ = true;
  last_response_info_ = info;
  last_error_ = error;
  SignalDone();
  MaybeCancelOrPause(request);
}

void TestUrlRequestCallback::OnCanceled(Cronet_UrlRequestPtr request,
                                        Cronet_UrlResponseInfoPtr info) {
  CheckExecutorThread();
  CHECK(Cronet_UrlRequest_IsDone(request));
  CHECK(!on_error_called_);
  // Should happen at most once for a single request.
  CHECK(!on_canceled_called_);
  CHECK(!last_error_);

  response_step_ = ON_CANCELED;
  last_response_info_ = info;
  on_canceled_called_ = true;
  SignalDone();
  MaybeCancelOrPause(request);
}

bool TestUrlRequestCallback::MaybeCancelOrPause(Cronet_UrlRequestPtr request) {
  CheckExecutorThread();
  if (response_step_ != failure_step_ || failure_type_ == NONE) {
    if (!auto_advance_) {
      step_block_.Signal();
      return true;
    }
    return false;
  }

  if (failure_type_ == CANCEL_SYNC) {
    Cronet_UrlRequest_Cancel(request);
  }

  /*
  Runnable task = new Runnable() {
      public void run() {
          request.cancel();
      }
  };
  if (failure_type_ == CANCEL_ASYNC
          || mFailureType == CANCEL_ASYNC_WITHOUT_PAUSE) {
      getExecutor().execute(task);
  } else {
      task.run();
  }*/
  return failure_type_ != CANCEL_ASYNC_WITHOUT_PAUSE;
}

/* static */
TestUrlRequestCallback* TestUrlRequestCallback::GetThis(
    Cronet_UrlRequestCallbackPtr self) {
  return static_cast<TestUrlRequestCallback*>(
      Cronet_UrlRequestCallback_GetContext(self));
}

/* static */
void TestUrlRequestCallback::OnRedirectReceived(
    Cronet_UrlRequestCallbackPtr self,
    Cronet_UrlRequestPtr request,
    Cronet_UrlResponseInfoPtr info,
    CharString newLocationUrl) {
  GetThis(self)->OnRedirectReceived(request, info, newLocationUrl);
}

/* static */
void TestUrlRequestCallback::OnResponseStarted(
    Cronet_UrlRequestCallbackPtr self,
    Cronet_UrlRequestPtr request,
    Cronet_UrlResponseInfoPtr info) {
  GetThis(self)->OnResponseStarted(request, info);
}

/* static */
void TestUrlRequestCallback::OnReadCompleted(Cronet_UrlRequestCallbackPtr self,
                                             Cronet_UrlRequestPtr request,
                                             Cronet_UrlResponseInfoPtr info,
                                             Cronet_BufferPtr buffer,
                                             uint64_t bytesRead) {
  GetThis(self)->OnReadCompleted(request, info, buffer, bytesRead);
}

/* static */
void TestUrlRequestCallback::OnSucceeded(Cronet_UrlRequestCallbackPtr self,
                                         Cronet_UrlRequestPtr request,
                                         Cronet_UrlResponseInfoPtr info) {
  GetThis(self)->OnSucceeded(request, info);
}

/* static */
void TestUrlRequestCallback::OnFailed(Cronet_UrlRequestCallbackPtr self,
                                      Cronet_UrlRequestPtr request,
                                      Cronet_UrlResponseInfoPtr info,
                                      Cronet_ErrorPtr error) {
  GetThis(self)->OnFailed(request, info, error);
}

/* static */
void TestUrlRequestCallback::OnCanceled(Cronet_UrlRequestCallbackPtr self,
                                        Cronet_UrlRequestPtr request,
                                        Cronet_UrlResponseInfoPtr info) {
  GetThis(self)->OnCanceled(request, info);
}

}  // namespace test
}  // namespace cronet
