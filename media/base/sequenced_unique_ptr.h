// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_BASE_SEQUENCED_UNIQUE_PTR_H_
#define MEDIA_BASE_SEQUENCED_UNIQUE_PTR_H_

#include "media/base/sequenced_scoped_ref_ptr.h"

namespace media {

// We're exactly the same as a SequencedScopedRefPtr, except that we are
// move-only.  We inherit privately to prevent casting and copying.
template <typename T>
class SequencedUniquePtr : private SequencedScopedRefPtr<T> {
 public:
  SequencedUniquePtr() : SequencedScopedRefPtr<T>() {}

  template <typename... Args>
  SequencedUniquePtr(scoped_refptr<base::SequencedTaskRunner> impl_task_runner,
                     Args... args)
      : SequencedScopedRefPtr<T>(std::move(impl_task_runner),
                                 std::move(args)...) {}

  SequencedUniquePtr(std::unique_ptr<T> t)
      : SequencedScopedRefPtr<T>(std::move(t)) {}

  SequencedUniquePtr(SequencedUniquePtr<T>&& other)
      : SequencedScopedRefPtr<T>(std::move(other)) {}

  SequencedWeakPtr<T> GetSequencedWeakPtr() {
    return SequencedScopedRefPtr<T>::GetSequencedWeakPtr();
  }

  SequencedUniquePtr& operator=(SequencedUniquePtr&& other) {
    SequencedScopedRefPtr<T>::operator=(std::move(other));
    return *this;
  }

  void reset() { SequencedScopedRefPtr<T>::reset(); }

  // This doesn't guarantee that the object has been constructed yet.  It just
  // guarantees that it will eventually.
  explicit operator bool() const {
    return SequencedScopedRefPtr<T>::operator bool();
  }

  T* GetRawPointerForTesting() const {
    return SequencedScopedRefPtr<T>::GetRawPointerForTesting();
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(SequencedUniquePtr);
};

}  // namespace media

#endif  // MEDIA_BASE_SEQUENCED_UNIQUE_PTR_H_
