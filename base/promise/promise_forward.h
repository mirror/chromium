// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_PROMISE_PROMISE_FORWARD_H_
#define BASE_PROMISE_PROMISE_FORWARD_H_

namespace base {

struct NotReached;

template <typename ResolveT, typename RejectT = void>
class Promise;

template <typename ResolveT, typename RejectT = void>
class Future;

}  // namespace base

#endif  // BASE_PROMISE_PROMISE_FORWARD_H_
