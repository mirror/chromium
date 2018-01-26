// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/base/test_completion_once_callback.h"

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/compiler_specific.h"
#include "base/run_loop.h"
#include "net/base/io_buffer.h"

namespace net {

namespace internal {

void TestCompletionOnceCallbackBaseInternal::DidSetResult() {
  have_result_ = true;
  if (run_loop_)
    run_loop_->Quit();
}

void TestCompletionOnceCallbackBaseInternal::WaitForResult() {
  DCHECK(!run_loop_);
  if (!have_result_) {
    run_loop_.reset(new base::RunLoop());
    run_loop_->Run();
    run_loop_.reset();
    DCHECK(have_result_);
  }
  have_result_ = false;  // Auto-reset for next callback.
}

TestCompletionOnceCallbackBaseInternal::TestCompletionOnceCallbackBaseInternal()
    : have_result_(false) {}

TestCompletionOnceCallbackBaseInternal::
    ~TestCompletionOnceCallbackBaseInternal() = default;

}  // namespace internal

TestOnceClosure::TestOnceClosure() = default;

TestOnceClosure::~TestOnceClosure() = default;

base::OnceClosure TestOnceClosure::CreateClosure() {
  return base::BindOnce(&TestOnceClosure::DidSetResult, base::Unretained(this));
}

TestCompletionOnceCallback::TestCompletionOnceCallback() = default;

TestCompletionOnceCallback::~TestCompletionOnceCallback() = default;

CompletionOnceCallback TestCompletionOnceCallback::CreateCallback() {
  return base::BindOnce(&TestCompletionOnceCallback::SetResult,
                        base::Unretained(this));
}

TestInt64CompletionOnceCallback::TestInt64CompletionOnceCallback() = default;

TestInt64CompletionOnceCallback::~TestInt64CompletionOnceCallback() = default;

Int64CompletionOnceCallback TestInt64CompletionOnceCallback::CreateCallback() {
  return base::BindOnce(&TestInt64CompletionOnceCallback::SetResult,
                        base::Unretained(this));
}

ReleaseBufferCompletionOnceCallback::ReleaseBufferCompletionOnceCallback(
    IOBuffer* buffer)
    : buffer_(buffer) {}

ReleaseBufferCompletionOnceCallback::~ReleaseBufferCompletionOnceCallback() =
    default;

void ReleaseBufferCompletionOnceCallback::SetResult(int result) {
  if (!buffer_->HasOneRef())
    result = ERR_FAILED;
  TestCompletionOnceCallback::SetResult(result);
}

}  // namespace net
