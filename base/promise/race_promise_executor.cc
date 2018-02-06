// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/promise/race_promise_executor.h"

namespace base {
namespace internal {

RacePromiseExecutor::RacePromiseExecutor() {}
RacePromiseExecutor::~RacePromiseExecutor() {}

void RacePromiseExecutor::Execute(AbstractPromise* promise) {
  bool success = false;
  for (const auto& prerequisite : promise->prerequisites()) {
    if (prerequisite->state() == AbstractPromise::State::RESOLVED) {
      success = promise->value().TryAssignOrUnwrapAndEmplace(
          prerequisite->value(), 1);
      DCHECK(success);
      break;
    } else if (prerequisite->state() == AbstractPromise::State::REJECTED) {
      success = promise->value().TryAssignOrUnwrapAndEmplace(
          prerequisite->value(), 2);
      DCHECK(success);
      break;
    }
  }
  DCHECK(success);
}

}  // namespace internal
}  // namespace base
