// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/task_scheduler/sequence_manager/cancelable_closure_holder.h"

namespace base {


CancelableClosureHolder::CancelableClosureHolder() {}

CancelableClosureHolder::~CancelableClosureHolder() {}

void CancelableClosureHolder::Reset(const base::Closure& callback) {
  callback_ = callback;
  cancelable_callback_.Reset(callback_);
}

void CancelableClosureHolder::Cancel() {
  DCHECK(!callback_.is_null());
  cancelable_callback_.Reset(callback_);
}

base::Closure CancelableClosureHolder::GetCallback() const {
  DCHECK(!callback_.is_null());
  return cancelable_callback_.callback();
}


}  // namespace base
