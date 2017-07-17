// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/memory/weak_ptr.h"

#include "base/debug/leak_annotations.h"

namespace base {
namespace internal {

static constexpr uintptr_t kTrueMask = ~static_cast<uintptr_t>(0);

WeakReference::Flag::Flag() : is_valid_(kTrueMask) {
#if DCHECK_IS_ON()
  // Flags only become bound when checked for validity, or invalidated,
  // so that we can check that later validity/invalidation operations on
  // the same Flag take place on the same sequenced thread.
  sequence_checker_.DetachFromSequence();
#endif
}

WeakReference::Flag::Flag(WeakReference::Flag::NullFlagTag) : is_valid_(false) {
  // There is no need for sequence_checker_.DetachFromSequence() because the
  // null flag doesn't participate in the sequence checks. See DCHECK in
  // Invalidate() and IsValid().

  // Keep the object alive perpetually, even when there are no references to it.
  AddRef();
}

WeakReference::Flag::~Flag() {}

WeakReference::Flag* WeakReference::Flag::NullFlag() {
  ANNOTATE_SCOPED_MEMORY_LEAK;
  static Flag* g_null_flag = new Flag(kNullFlagTag);
  return g_null_flag;
}

void WeakReference::Flag::Invalidate() {
#if DCHECK_IS_ON()
  if (this == NullFlag()) {
    // The Null Flag does not participate in the sequence checks below.
    // Since its state never changes, it can be accessed from any thread.
    DCHECK(!is_valid_);
    return;
  }
  DCHECK(sequence_checker_.CalledOnValidSequence())
      << "WeakPtrs must be invalidated on the same sequenced thread.";
#endif
  is_valid_ = 0;
}

void WeakReference::Flag::DetachFromSequence() {
  DCHECK(HasOneRef()) << "Cannot detach from Sequence while WeakPtrs exist.";
  sequence_checker_.DetachFromSequence();
}

WeakReference::WeakReference() : flag_(Flag::NullFlag()) {}

WeakReference::WeakReference(const scoped_refptr<Flag>& flag) : flag_(flag) {}

WeakReference::~WeakReference() {}

WeakReference::WeakReference(WeakReference&& other)
    : flag_(std::move(other.flag_)) {
  other.flag_ = Flag::NullFlag();
}

WeakReference::WeakReference(const WeakReference& other) = default;

WeakReferenceOwner::WeakReferenceOwner()
    : flag_(WeakReference::Flag::NullFlag()) {}

WeakReferenceOwner::~WeakReferenceOwner() {
  Invalidate();
}

WeakReference WeakReferenceOwner::GetRef() const {
#if DCHECK_IS_ON()
  if (flag_ != WeakReference::Flag::NullFlag())
    DCHECK(flag_->IsValid());
#endif
  if (flag_ == WeakReference::Flag::NullFlag())
    flag_ = make_scoped_refptr(new WeakReference::Flag());

  return WeakReference(flag_);
}

void WeakReferenceOwner::Invalidate() {
  if (flag_ != WeakReference::Flag::NullFlag()) {
    flag_->Invalidate();
    flag_ = WeakReference::Flag::NullFlag();
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
