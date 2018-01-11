// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_PROMISE_RACE_PROMISE_EXECUTOR_H_
#define BASE_PROMISE_RACE_PROMISE_EXECUTOR_H_

#include "base/promise/abstract_promise.h"

namespace base {
namespace internal {

class BASE_EXPORT RacePromiseExecutor : public AbstractPromise::Executor {
 public:
  RacePromiseExecutor();
  ~RacePromiseExecutor() override;

  void Execute(AbstractPromise* promise) override;
};

}  // namespace internal
}  // namespace base

#endif  // BASE_PROMISE_RACE_PROMISE_EXECUTOR_H_
