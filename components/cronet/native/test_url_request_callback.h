// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_CRONET_NATIVE_TEST_URL_REQUEST_CALLBACK_H_
#define COMPONENTS_CRONET_NATIVE_TEST_URL_REQUEST_CALLBACK_H_

#include <string>
#include <vector>

#include "cronet_c.h"

#include "base/macros.h"
#include "base/synchronization/lock.h"
#include "base/synchronization/waitable_event.h"

namespace cronet {
// Various test utility functions for testing Cronet.
namespace test {

class TestUrlRequestCallback {
 public:
  enum ResponseStep {
    NOTHING,
    ON_RECEIVED_REDIRECT,
    ON_RESPONSE_STARTED,
    ON_READ_COMPLETED,
    ON_SUCCEEDED,
    ON_FAILED,
    ON_CANCELED,
  };

  enum FailureType {
    NONE,
    CANCEL_SYNC,
    CANCEL_ASYNC,
    // Same as above, but continues to advance the request after posting
    // the cancellation task.
    CANCEL_ASYNC_WITHOUT_PAUSE,
    THROW_SYNC
  };

  std::vector<std::string> redirect_url_list_;
  // Owned by UrlRequest, only valid until UrlRequest is destroyed.
  Cronet_UrlResponseInfoPtr last_response_info_ = nullptr;
  // Owned by UrlRequest, only valid until UrlRequest is destroyed.
  Cronet_ErrorPtr last_error_ = nullptr;

  ResponseStep response_step_ = NOTHING;

  int redirect_count_ = 0;
  bool on_error_called_ = false;
  bool on_canceled_called_ = false;

  int response_data_length_ = 0;
  std::string response_as_string_;

  TestUrlRequestCallback();
  ~TestUrlRequestCallback();

  Cronet_UrlRequestCallbackPtr CreateUrlRequestCallback();

  void set_auto_advance(bool auto_advance) { auto_advance_ = auto_advance; }

  void set_allow_direct_executor(bool allowed) {
    allow_direct_executor_ = allowed;
  }

  void set_accumulate_response_data(bool accuumulate) {
    accumulate_response_data_ = accuumulate;
  }

  void set_failure(FailureType failure_type, ResponseStep failure_step) {
    failure_step_ = failure_step;
    failure_type_ = failure_type;
  }

  void WaitForDone() { done_.Wait(); }

  void WaitForNextStep() {
    step_block_.Wait();
    step_block_.Reset();
  }

  bool IsDone() { return done_.IsSignaled(); }

 protected:
  virtual void OnRedirectReceived(Cronet_UrlRequestPtr request,
                                  Cronet_UrlResponseInfoPtr info,
                                  CharString newLocationUrl);

  virtual void OnResponseStarted(Cronet_UrlRequestPtr request,
                                 Cronet_UrlResponseInfoPtr info);

  virtual void OnReadCompleted(Cronet_UrlRequestPtr request,
                               Cronet_UrlResponseInfoPtr info,
                               Cronet_BufferPtr buffer,
                               uint64_t bytes_read);

  virtual void OnSucceeded(Cronet_UrlRequestPtr request,
                           Cronet_UrlResponseInfoPtr info);

  virtual void OnFailed(Cronet_UrlRequestPtr request,
                        Cronet_UrlResponseInfoPtr info,
                        Cronet_ErrorPtr error);

  virtual void OnCanceled(Cronet_UrlRequestPtr request,
                          Cronet_UrlResponseInfoPtr info);

  void StartNextRead(Cronet_UrlRequestPtr request) {
    Cronet_BufferPtr buffer = Cronet_Buffer_Create();
    Cronet_Buffer_InitWithAlloc(buffer, READ_BUFFER_SIZE);

    StartNextRead(request, buffer);
  }

  void StartNextRead(Cronet_UrlRequestPtr request, Cronet_BufferPtr buffer) {
    Cronet_UrlRequest_Read(request, buffer);
  }

  void SignalDone() { done_.Signal(); }

  void CheckExecutorThread() {
    /*
    if (!mAllowDirectExecutor) {
        assertEquals(mExecutorThread, Thread.currentThread());
    }
    */
  }

  /**
   * Returns false if the callback should continue to advance the
   * request.
   */
  bool MaybeCancelOrPause(Cronet_UrlRequestPtr request);

  static TestUrlRequestCallback* GetThis(Cronet_UrlRequestCallbackPtr self);

  static void OnRedirectReceived(Cronet_UrlRequestCallbackPtr self,
                                 Cronet_UrlRequestPtr request,
                                 Cronet_UrlResponseInfoPtr info,
                                 CharString newLocationUrl);

  static void OnResponseStarted(Cronet_UrlRequestCallbackPtr self,
                                Cronet_UrlRequestPtr request,
                                Cronet_UrlResponseInfoPtr info);

  static void OnReadCompleted(Cronet_UrlRequestCallbackPtr self,
                              Cronet_UrlRequestPtr request,
                              Cronet_UrlResponseInfoPtr info,
                              Cronet_BufferPtr buffer,
                              uint64_t bytesRead);

  static void OnSucceeded(Cronet_UrlRequestCallbackPtr self,
                          Cronet_UrlRequestPtr request,
                          Cronet_UrlResponseInfoPtr info);

  static void OnFailed(Cronet_UrlRequestCallbackPtr self,
                       Cronet_UrlRequestPtr request,
                       Cronet_UrlResponseInfoPtr info,
                       Cronet_ErrorPtr error);

  static void OnCanceled(Cronet_UrlRequestCallbackPtr self,
                         Cronet_UrlRequestPtr request,
                         Cronet_UrlResponseInfoPtr info);

  const int READ_BUFFER_SIZE = 32 * 1024;

  // When false, the consumer is responsible for all calls into the request
  // that advance it.
  bool auto_advance_ = true;

  // When false response data is not accuumulated for better performance.
  bool accumulate_response_data_ = true;

  // Whether to permit calls on the network thread.
  bool allow_direct_executor_ = false;

  // Conditionally fail on certain steps.
  FailureType failure_type_ = NONE;
  ResponseStep failure_step_ = NOTHING;

  // Signals when request is done either successfully or not.
  base::WaitableEvent done_;

  // Signaled on each step when |auto_advance_| is false.
  base::WaitableEvent step_block_;
};

}  // namespace test
}  // namespace cronet

#endif  // COMPONENTS_CRONET_NATIVE_TEST_URL_REQUEST_CALLBACK_H_
