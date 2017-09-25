// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// DEFER() macro allows to call arbitrary callable functor on scope exit:
//
// void Foo() {}
// void Bar() {
//   DEFER(Foo);  // Calls Foo when function exits.
// }
//
// void Baz() {
//   FILE* f = fopen("/dev/null", "r");
//   DEFER([&f]() { fclose(f); });  // Closes the file handle.
// }

#ifndef BASE_DEFER_H_
#define BASE_DEFER_H_

#include <utility>

#define DEFER_INTERNAL_CAT2(x, y) x##y
#define DEFER_INTERNAL_CAT(x, y) DEFER_INTERNAL_CAT2(x, y)

#define DEFER(f)                                                     \
  const auto DEFER_INTERNAL_CAT(DEFER_INTERNAL_VAR_NAME, __LINE__) = \
      base::internal::Defer(f)

namespace base {

namespace internal {

template <class F>
class DeferredAction {
 public:
  DeferredAction() = delete;
  explicit DeferredAction(F action) : action_(std::move(action)) {}
  DeferredAction(const DeferredAction&) = delete;
  DeferredAction(DeferredAction&&) = default;

  ~DeferredAction() { action_(); }

  DeferredAction& operator=(const DeferredAction&) = delete;

 private:
  F action_;
};

template <class F>
inline DeferredAction<F> Defer(F&& f) {
  return DeferredAction<F>(std::forward<F>(f));
}

}  // namespace internal

}  // namespace base

#endif  // BASE_DEFER_H_
