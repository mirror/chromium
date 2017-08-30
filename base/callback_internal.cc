// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/callback_internal.h"

#include "base/logging.h"

namespace base {
namespace internal {

namespace {

bool ReturnFalse(const BindStateBase*) {
  return false;
}

void ReleaseBindStateBaseRefCounted(const BindStateBase* bind_state) {
  static_cast<const BindStateBaseRefCounted*>(bind_state)->Release();
}

}  // namespace

void BindStateBaseReleaser::operator()(BindStateBase* bind_state) const {
  bind_state->releaser_(bind_state);
}

// static
void BindStateBaseRefCountTraits::Destruct(
    const BindStateBaseRefCounted* bind_state) {
  bind_state->destructor_(bind_state);
}

BindStateBase::BindStateBase(InvokeFuncStorage polymorphic_invoke,
                             void (*releaser)(const BindStateBase*))
    : BindStateBase(polymorphic_invoke, releaser, &ReturnFalse) {}

BindStateBase::BindStateBase(InvokeFuncStorage polymorphic_invoke,
                             void (*releaser)(const BindStateBase*),
                             bool (*is_cancelled)(const BindStateBase*))
    : polymorphic_invoke_(polymorphic_invoke),
      releaser_(releaser),
      is_cancelled_(is_cancelled) {}

BindStateBaseRefCounted::BindStateBaseRefCounted(
    InvokeFuncStorage polymorphic_invoke,
    void (*destructor)(const BindStateBase*))
    : BindStateBase(polymorphic_invoke,
                    &ReleaseBindStateBaseRefCounted,
                    &ReturnFalse),
      destructor_(destructor) {}

BindStateBaseRefCounted::BindStateBaseRefCounted(
    InvokeFuncStorage polymorphic_invoke,
    void (*destructor)(const BindStateBase*),
    bool (*is_cancelled)(const BindStateBase*))
    : BindStateBase(polymorphic_invoke,
                    &ReleaseBindStateBaseRefCounted,
                    is_cancelled),
      destructor_(destructor) {}

// static
BindStateBasePtr BindStateBaseRefCounted::AsBindStateBase(
    scoped_refptr<BindStateBaseRefCounted> bind_state) {
  return BindStateBasePtr(bind_state.LeakRef());
}

CallbackBase::CallbackBase(CallbackBase&&) = default;
CallbackBase& CallbackBase::operator=(CallbackBase&&) = default;

CallbackBase::CallbackBase(const CallbackBaseCopyable& other)
    : bind_state_(BindStateBaseRefCounted::AsBindStateBase(other.bind_state_)) {
}

CallbackBase& CallbackBase::operator=(const CallbackBaseCopyable& other) {
  bind_state_ = BindStateBaseRefCounted::AsBindStateBase(other.bind_state_);
  return *this;
}

CallbackBase::CallbackBase(CallbackBaseCopyable&& other)
    : bind_state_(BindStateBaseRefCounted::AsBindStateBase(
          std::move(other.bind_state_))) {}

CallbackBase& CallbackBase::operator=(CallbackBaseCopyable&& other) {
  bind_state_ =
      BindStateBaseRefCounted::AsBindStateBase(std::move(other.bind_state_));
  return *this;
}

void CallbackBase::Reset() {
  bind_state_ = nullptr;
}

bool CallbackBase::IsCancelled() const {
  DCHECK(bind_state_);
  return bind_state_->IsCancelled();
}

bool CallbackBase::EqualsInternal(const CallbackBase& other) const {
  return bind_state_ == other.bind_state_;
}

CallbackBase::CallbackBase(BindStateBase* bind_state)
    : bind_state_(bind_state) {}

CallbackBase::~CallbackBase() {}

CallbackBaseCopyable::CallbackBaseCopyable(const CallbackBaseCopyable&) =
    default;
CallbackBaseCopyable::CallbackBaseCopyable(CallbackBaseCopyable&&) = default;

CallbackBaseCopyable& CallbackBaseCopyable::operator=(
    const CallbackBaseCopyable& other) = default;
CallbackBaseCopyable& CallbackBaseCopyable::operator=(
    CallbackBaseCopyable&& other) = default;

bool CallbackBaseCopyable::IsCancelled() const {
  DCHECK(bind_state_);
  return bind_state_->IsCancelled();
}

void CallbackBaseCopyable::Reset() {
  bind_state_ = nullptr;
}

bool CallbackBaseCopyable::EqualsInternal(
    const CallbackBaseCopyable& other) const {
  return bind_state_ == other.bind_state_;
}

CallbackBaseCopyable::CallbackBaseCopyable(BindStateBaseRefCounted* bind_state)
    : bind_state_(bind_state ? AdoptRef(bind_state) : nullptr) {
  DCHECK(!bind_state_ || bind_state_->HasOneRef());
}

CallbackBaseCopyable::~CallbackBaseCopyable() {}

}  // namespace internal
}  // namespace base
