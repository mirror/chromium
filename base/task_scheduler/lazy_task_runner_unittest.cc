// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "testing/gtest/include/gtest/gtest.h"

#include "base/sequenced_task_runner.h"
#include "base/task_scheduler/lazy_task_runner.h"

namespace base {

namespace {
/*
LazySingleThreadTaskRunner g_lazy_sequenced_task_runner(
    TaskTraits(MayBlock()));*/

internal::LazyTaskRunner<SequencedTaskRunner> g_toto(base::TaskTraits());

}  // namespace

TEST(LazyTaskRunner, LazyTaskRunner) {
  auto v = g_toto.Get();
  LOG(ERROR) << v.get();
}

}  // namespace base
