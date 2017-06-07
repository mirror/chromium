// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/memory/weak_ptr.h"

namespace base {
namespace internal {

WeakReference::Flag::Flag() {
  // Flags only become bound when checked for validity, or invalidated,
  // so that we can check that later validity/invalidation operations on
  // the same Flag take place on the same sequence.
  sequence_checker_.DetachFromSequence();
}

void WeakReference::Flag::Invalidate() {
  // The flag being invalidated with a single ref implies that there are no
  // weak pointers in existence. Allow deletion on other sequence in this case.
  DCHECK(sequence_checker_.CalledOnValidSequence() || HasOneRef())
      << "WeakPtrs must be invalidated on the same sequence.";
  invalidated_.Set();
}

bool WeakReference::Flag::IsValid() const {
  DCHECK(sequence_checker_.CalledOnValidSequence())
      << "WeakPtrs must be used on the same sequence.";
  return !invalidated_.IsSet();
}

bool WeakReference::Flag::IsValidThreadSafe() const {
  return !invalidated_.IsSet();
}

WeakReference::WeakReference() {
}

WeakReference::WeakReference(scoped_refptr<const Flag> flag)
    : flag_(std::move(flag)) {}

WeakReference::~WeakReference() {
}

WeakReference::WeakReference(WeakReference&& other) = default;

WeakReference::WeakReference(const WeakReference& other) = default;

bool WeakReference::IsValid() const {
  return flag_.get() && flag_->IsValid();
}

bool WeakReference::IsValidThreadSafe() const {
  return flag_.get() && flag_->IsValidThreadSafe();
}

WeakReferenceOwner::WeakReferenceOwner() {
}

WeakReferenceOwner::~WeakReferenceOwner() {
  Invalidate();
}

WeakReference WeakReferenceOwner::GetRef() const {
  // If we hold the last reference to the Flag then create a new one.
  if (!HasRefs())
    flag_ = new WeakReference::Flag();

  return WeakReference(flag_);
}

void WeakReferenceOwner::Invalidate() {
  if (flag_.get()) {
    flag_->Invalidate();
    flag_ = nullptr;
  }
}

WeakPtrBase::WeakPtrBase() {
}

WeakPtrBase::~WeakPtrBase() {
}

WeakPtrBase::WeakPtrBase(const WeakReference& ref) : ref_(ref) {
}

}  // namespace internal
}  // namespace base
