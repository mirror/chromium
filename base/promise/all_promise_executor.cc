// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/promise/all_promise_executor.h"

namespace base {
namespace internal {

AllPromiseExecutor::AllPromiseExecutor(
    std::unique_ptr<TupleConstructor> tuple_constructor)
    : tuple_constructor_(std::move(tuple_constructor)) {}
AllPromiseExecutor::~AllPromiseExecutor() {}

void AllPromiseExecutor::Execute(AbstractPromise* promise) {
  for (const auto& prerequisite : promise->prerequisites()) {
    if (prerequisite->state() == AbstractPromise::State::REJECTED) {
      bool success = promise->value().TryAssignOrUnwrapAndEmplace(
          prerequisite->value(), 2);
      DCHECK(success);
      return;
    }
  }

  tuple_constructor_->ConstructTuple(promise->prerequisites(),
                                     &promise->value());
}

}  // namespace internal
}  // namespace base
