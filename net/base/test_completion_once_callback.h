// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_BASE_TEST_COMPLETION_ONCE_CALLBACK_H_
#define NET_BASE_TEST_COMPLETION_ONCE_CALLBACK_H_

#include <stdint.h>

#include <memory>

#include "base/callback.h"
#include "base/compiler_specific.h"
#include "base/macros.h"
#include "net/base/completion_once_callback.h"
#include "net/base/net_errors.h"

//-----------------------------------------------------------------------------
// completion callback helper

// A helper class for completion callbacks, designed to make it easy to run
// tests involving asynchronous operations.  Just call WaitForResult to wait
// for the asynchronous operation to complete.  Uses a RunLoop to spin the
// current MessageLoop while waiting.  The callback must be invoked on the same
// thread WaitForResult is called on.
//
// NOTE: Since this runs a message loop to wait for the completion callback,
// there could be other side-effects resulting from WaitForResult.  For this
// reason, this class is probably not ideal for a general application.
//
namespace base {
class RunLoop;
}

namespace net {

class IOBuffer;

namespace internal {

class TestCompletionOnceCallbackBaseInternal {
 public:
  bool have_result() const { return have_result_; }

 protected:
  TestCompletionOnceCallbackBaseInternal();
  virtual ~TestCompletionOnceCallbackBaseInternal();

  void DidSetResult();
  void WaitForResult();

 private:
  // RunLoop.  Only non-NULL during the call to WaitForResult, so the class is
  // reusable.
  std::unique_ptr<base::RunLoop> run_loop_;
  bool have_result_;

  DISALLOW_COPY_AND_ASSIGN(TestCompletionOnceCallbackBaseInternal);
};

template <typename R>
class TestCompletionOnceCallbackTemplate
    : public TestCompletionOnceCallbackBaseInternal {
 public:
  virtual ~TestCompletionOnceCallbackTemplate() override {}

  R WaitForResult() {
    TestCompletionOnceCallbackBaseInternal::WaitForResult();
    return result_;
  }

  R GetResult(R result) {
    if (ERR_IO_PENDING != result)
      return result;
    return WaitForResult();
  }

 protected:
  TestCompletionOnceCallbackTemplate() : result_(R()) {}

  // Override this method to gain control as the callback is running.
  virtual void SetResult(R result) {
    result_ = result;
    DidSetResult();
  }

 private:
  R result_;

  DISALLOW_COPY_AND_ASSIGN(TestCompletionOnceCallbackTemplate);
};

}  // namespace internal

class TestOnceClosure
    : public internal::TestCompletionOnceCallbackBaseInternal {
 public:
  using internal::TestCompletionOnceCallbackBaseInternal::WaitForResult;

  TestOnceClosure();
  ~TestOnceClosure() override;

  base::OnceClosure CreateClosure();

 private:
  DISALLOW_COPY_AND_ASSIGN(TestOnceClosure);
};

// Base class overridden by custom implementations of
// TestCompletionOnceCallback.
typedef internal::TestCompletionOnceCallbackTemplate<int>
    TestCompletionOnceCallbackBase;

typedef internal::TestCompletionOnceCallbackTemplate<int64_t>
    TestInt64CompletionOnceCallbackBase;

class TestCompletionOnceCallback : public TestCompletionOnceCallbackBase {
 public:
  TestCompletionOnceCallback();
  ~TestCompletionOnceCallback() override;

  CompletionOnceCallback CreateCallback();

 private:
  DISALLOW_COPY_AND_ASSIGN(TestCompletionOnceCallback);
};

class TestInt64CompletionOnceCallback
    : public TestInt64CompletionOnceCallbackBase {
 public:
  TestInt64CompletionOnceCallback();
  ~TestInt64CompletionOnceCallback() override;

  Int64CompletionOnceCallback CreateCallback();

 private:
  DISALLOW_COPY_AND_ASSIGN(TestInt64CompletionOnceCallback);
};

// Makes sure that the buffer is not referenced when the callback runs.
class ReleaseBufferCompletionOnceCallback : public TestCompletionOnceCallback {
 public:
  explicit ReleaseBufferCompletionOnceCallback(IOBuffer* buffer);
  ~ReleaseBufferCompletionOnceCallback() override;

 private:
  void SetResult(int result) override;

  IOBuffer* buffer_;
  DISALLOW_COPY_AND_ASSIGN(ReleaseBufferCompletionOnceCallback);
};

}  // namespace net

#endif  // NET_BASE_TEST_COMPLETION_ONCE_CALLBACK_H_
