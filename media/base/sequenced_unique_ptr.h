// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_BASE_SEQUENCED_UNIQUE_PTR_H_
#define MEDIA_BASE_SEQUENCED_UNIQUE_PTR_H_

#include "media/base/sequenced_scoped_ref_ptr.h"

namespace media {

// We're exactly the same as a SequencedScopedRefPtr, except that we are
// move-only.  We inherit privately to prevent casting and copying.
// TODO(liberato): we can't be a weak ref provider if scoped refptr is, but we
// inherit privately from it.  we need a base that is the holder.  then, we can
// derive both refptr and unique_ptr from the base.  each of them would
// implement the provider separately.
template <typename T>
class SequencedUniquePtr : private SequencedScopedRefPtrBase<T>,
                           public TypedInternalWeakRefProvider<T> {
 public:
  SequencedUniquePtr()
      : SequencedScopedRefPtrBase<T>(),
        TypedInternalWeakRefProvider<T>(
            *static_cast<InternalWeakRefHolder*>(this)) {}

  template <typename Impl, typename... Args>
  static SequencedUniquePtr<T> Create(
      scoped_refptr<base::SequencedTaskRunner> impl_task_runner,
      Args... args) {
    return SequencedUniquePtr<T>(
        impl_task_runner,
        base::BindOnce(&internal::SequencedFactory<T, Impl, Args...>,
                       std::move(args)...));
  }

 protected:
  SequencedUniquePtr(scoped_refptr<base::SequencedTaskRunner> impl_task_runner,
                     base::OnceCallback<std::unique_ptr<T>()> factory_cb)
      : SequencedScopedRefPtrBase<T>(std::move(impl_task_runner),
                                     std::move(factory_cb)),
        TypedInternalWeakRefProvider<T>(
            *static_cast<InternalWeakRefHolder*>(this)) {}

 public:
  template <typename... Args>
  SequencedUniquePtr(scoped_refptr<base::SequencedTaskRunner> impl_task_runner,
                     Args... args)
      : SequencedScopedRefPtrBase<T>(std::move(impl_task_runner),
                                     std::move(args)...),
        TypedInternalWeakRefProvider<T>(
            *static_cast<InternalWeakRefHolder*>(this)) {}

  SequencedUniquePtr(std::unique_ptr<T> t)
      : SequencedScopedRefPtrBase<T>(std::move(t)),
        TypedInternalWeakRefProvider<T>(
            *static_cast<InternalWeakRefHolder*>(this)) {}

  SequencedUniquePtr(SequencedUniquePtr<T>&& other)
      : SequencedScopedRefPtrBase<T>(std::move(other)),
        TypedInternalWeakRefProvider<T>(
            *static_cast<InternalWeakRefHolder*>(this)) {
    std::move(other);  // ?
  }

  template <typename U>
  SequencedUniquePtr(SequencedUniquePtr<U>&& other)
      : SequencedScopedRefPtrBase<T>(
            std::move(static_cast<SequencedScopedRefPtrBase<U>&&>(other))),
        TypedInternalWeakRefProvider<T>(
            *static_cast<InternalWeakRefHolder*>(this)) {
    std::move(other);  // ?
  }

  SequencedUniquePtr& operator=(SequencedUniquePtr&& other) {
    SequencedScopedRefPtrBase<T>::operator=(std::move(other));
    return *this;
  }

  template <typename U>
  SequencedUniquePtr& operator=(SequencedUniquePtr<U>&& other) {
    SequencedScopedRefPtrBase<T>::operator=(std::move(other));
    return *this;
  }

  template <typename... Args>
  void Post(const base::Location& location, Args... args) {
    SequencedWeakPtr<T>(*this).Post(location, std::move(args)...);
    // GetSequencedWeakPtr().Post(location, std::move(args)...);
  }

  void reset() { SequencedScopedRefPtrBase<T>::reset(); }

  // This doesn't guarantee that the object has been constructed yet.  It just
  // guarantees that it will eventually.
  explicit operator bool() const {
    return SequencedScopedRefPtrBase<T>::operator bool();
  }

  T* GetRawPointerForTesting() const {
    return SequencedScopedRefPtrBase<T>::GetRawPointerForTesting();
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(SequencedUniquePtr);
};

}  // namespace media

#endif  // MEDIA_BASE_SEQUENCED_UNIQUE_PTR_H_
