// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef GPU_COMMAND_BUFFER_COMMON_CONTEXT_RESULT_H_
#define GPU_COMMAND_BUFFER_COMMON_CONTEXT_RESULT_H_

namespace gpu {

// The result of trying to create a gpu context.
enum class ContextResult {
  // The context was made successfully.
  kSuccess,
  // A failure occured that prevented the context from being initialized,
  // but it can be retried and expect to make progress.
  kTransientFailure,
  // Anerror occured that will recur in future attempts too with the
  // same inputs, retrying would not be productive.
  kFatalFailure,
  kLastContextResult = kFatalFailure
};

}  // namespace gpu

#endif  // GPU_COMMAND_BUFFER_COMMON_CONTEXT_RESULT_H_
