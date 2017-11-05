// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/memory/weak_ptr.h"

namespace base {
namespace internal {

WeakReference::Flag::Flag() : is_valid_(true) {
  // Flags only become bound when checked for validity, or invalidated,
  // so that we can check that later validity/invalidation operations on
  // the same Flag take place on the same sequenced thread.
  sequence_checker_.DetachFromSequence();
}

void WeakReference::Flag::Invalidate() {
  DCHECK(sequence_checker_.CalledOnValidSequence())
      << "WeakPtrs must be invalidated on the same sequenced thread.";
  is_valid_ = false;
}

bool WeakReference::Flag::IsValid() const {
  DCHECK(sequence_checker_.CalledOnValidSequence())
      << "WeakPtrs must be checked on the same sequenced thread.";
  return is_valid_;
}

WeakReference::Flag::~Flag() {
}

void WeakReference::Flag::DetachFromSequence() {
  DCHECK(HasOneRef()) << "Cannot detach from Sequence while WeakPtrs exist.";
  sequence_checker_.DetachFromSequence();
}

WeakReference::WeakReference() {}

WeakReference::WeakReference(const scoped_refptr<Flag>& flag) : flag_(flag) {}

WeakReference::~WeakReference() {
}

WeakReference::WeakReference(WeakReference&& other) = default;

WeakReference::WeakReference(const WeakReference& other) = default;

bool WeakReference::is_valid() const {
  return flag_ && flag_->IsValid();
}

WeakReferenceOwner::WeakReferenceOwner() {
}

WeakReferenceOwner::~WeakReferenceOwner() {
  Invalidate();
}

WeakReference WeakReferenceOwner::GetRef() const {
  if (!flag_)
    flag_ = new WeakReference::Flag();

  return WeakReference(flag_);
}

void WeakReferenceOwner::Invalidate() {
  if (flag_) {
    flag_->Invalidate();
    flag_ = nullptr;
  }
}

void WeakReferenceOwner::DetachFromSequence() {
  flag_->DetachFromSequence();
}

WeakPtrBase::WeakPtrBase() : ptr_(0) {}

WeakPtrBase::~WeakPtrBase() {}

WeakPtrBase::WeakPtrBase(const WeakReference& ref, uintptr_t ptr)
    : ref_(ref), ptr_(ptr) {}

WeakPtrFactoryBase::WeakPtrFactoryBase(uintptr_t ptr) : ptr_(ptr) {}

WeakPtrFactoryBase::~WeakPtrFactoryBase() {
  ptr_ = 0;
}

}  // namespace internal
}  // namespace base
