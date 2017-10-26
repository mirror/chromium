// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_BASE_SEQUENCED_WEAK_PTR_H_
#define MEDIA_BASE_SEQUENCED_WEAK_PTR_H_

#include "base/bind.h"
#include "base/memory/ptr_util.h"
#include "base/sequenced_task_runner.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "base/unbindable_as_pointer.h"
#include "media/base/bind_to_current_loop.h"
#include "media/base/sequenced_object.h"

namespace media {

template <typename T>
class SequencedScopedRefPtr;

// A SequencedWeakPtr is the only way for something that does't own the
// object to call methods on it.  All method invocations will be run
// asynchronously, on the correct sequence for the SequencedObject.
//
// If the object is destroyed before the callback runs, then it will be
// cancelled automatically.
//
// Callbacks are not tied to the SequencedWeakPtr instance that created
// them.  It's legal to do this:
//
// SequencedWeakPtr<Foo>(&foo).BindOnce(...);
template <typename T>
class SequencedWeakPtr {
 public:
  SequencedWeakPtr() {}

  // These must be called from the impl sequence.  However, since nothing but
  // the impl sequence should have |T|, that's fine.
  explicit SequencedWeakPtr(T& thiz) : SequencedWeakPtr(&thiz) {}
  explicit SequencedWeakPtr(std::unique_ptr<T>& thiz)
      : SequencedWeakPtr(thiz.get()) {}
  explicit SequencedWeakPtr(T* thiz)
      : impl_task_runner_(static_cast<SequencedObject<T>*>(thiz)->task_runner_),
        weak_ref_(thiz->GetWeakRef()) {}

  // This one must be thread-safe and may be called from anywhere.
  SequencedWeakPtr(const SequencedWeakPtr& other)
      : impl_task_runner_(other.impl_task_runner_),
        weak_ref_(other.weak_ref_) {}

  SequencedWeakPtr(SequencedWeakPtr&& other)
      : impl_task_runner_(std::move(other.impl_task_runner_)),
        weak_ref_(std::move(other.weak_ref_)) {}

  SequencedWeakPtr& operator=(const SequencedWeakPtr& other) {
    impl_task_runner_ = other.impl_task_runner_;
    weak_ref_ = other.weak_ref_;
    return *this;
  }

  // Allow casting from some more-derived class |U| of |T|.
  template <typename U>
  SequencedWeakPtr(const SequencedWeakPtr<U>& other) {
    impl_task_runner_ = other.impl_task_runner_;
    weak_ref_ = other.weak_ref_;
  }

  template <typename U>
  SequencedWeakPtr(SequencedWeakPtr<U>&& other) {
    impl_task_runner_ = std::move(other.impl_task_runner_);
    weak_ref_ = std::move(other.weak_ref_);
  }

  // Calls to |wp_| on some other thread.
  template <typename Method, typename... Args>
  void Post(const base::Location& location, Method method, Args... args) {
    // |location| is ignored, unfortunately.
    std::move(BindOnce(method)).Run(std::forward<Args>(args)...);
  }

 private:
  // Helper class to hide a WeakPtr<UnbindableAsPointer> type, since we can't
  // bind it directly.  The Run method will have the same signature as |Method|,
  // but will forward the call to the original method via |wp|.
  // TODO(liberato): I believe that we can simplify this now that we're not
  // actually sending base::WeakPtrs around.
  template <typename Method>
  struct HiddenWeakPointer;

  // Private constructor for deferred SWPs.  This is useful when we know that a
  // wp will exist on |impl_task_runner|, but we don't know what it is yet.  Our
  // caller should provide the InternalWeakRef for us to use, and it should also
  // guarantee that it will be filled in with a valid pointer on the impl thread
  // before it's used.
  SequencedWeakPtr(
      scoped_refptr<base::SequencedTaskRunner> impl_task_runner,
      scoped_refptr<typename SequencedObject<T>::InternalWeakRef> weak_ref)
      : impl_task_runner_(std::move(impl_task_runner)),
        weak_ref_(std::move(weak_ref)) {}

  // Note that we do not assume that |Class| is |T|, for wrapped objects.
  template <typename Class, typename... Args>
  struct HiddenWeakPointer<void (Class::*)(Args...)> {
    scoped_refptr<typename SequencedObject<
        typename Class::SequencedObjectBaseType>::InternalWeakRef>
        weak_ref;

    void (Class::*method_ptr)(Args...);
    void Run(Args... args) {
      // Since |Class| might be derived from the base type, we cast it back.
      // Note that this doesn't bother to check if the weak ptr is actually a
      // ptr to this type -- one might use any derived method from any class
      // that derives from A, even if this is a weakptr to some other type of A.
      // TODO(liberato): fix this.
      if (weak_ref->ptr)
        (static_cast<Class*>(weak_ref->ptr)->*method_ptr)(
            std::forward<Args>(args)...);
    }
  };

  // TODO(liberato): How do we handle const-ness without duplicating this?
  template <typename Class, typename... Args>
  struct HiddenWeakPointer<void (Class::*)(Args...) const> {
    scoped_refptr<typename SequencedObject<
        typename Class::SequencedObjectBaseType>::InternalWeakRef>
        weak_ref;

    void (Class::*method_ptr)(Args...) const;
    void Run(Args... args) {
      if (weak_ref->ptr)
        (static_cast<const Class*>(weak_ref->ptr)->*method_ptr)(
            std::forward<Args>(args)...);
    }
  };

 public:
  // Create a thread-safe callback.  It would be nice if Bind did this
  // automatically if given a SequencedWeakPtr.
  template <typename Method, typename... Args>
  auto BindOnce(Method method, Args... args) {
    // |closure| will take whatever arguments are still unbound.
    // We have to hide |wp_| since it can't be bound directly.
    // TODO(liberato): if we rename Run() as operator(), can we just pass it
    // by value instead?
    using HWP = HiddenWeakPointer<Method>;
    HWP* hwp = new HWP;
    hwp->method_ptr = method;
    hwp->weak_ref = weak_ref_;
    auto closure =
        base::BindOnce(&HWP::Run, base::Owned(hwp), std::move(args)...);
    return BindToLoop(impl_task_runner_, std::move(closure));
  }

  // Create a thread-safe callback.
  template <typename Method, typename... Args>
  auto BindRepeating(Method method, Args... args) {
    // |closure| will take whatever arguments are still unbound.
    using HWP = HiddenWeakPointer<Method>;
    HWP* hwp = new HWP;
    hwp->method_ptr = method;
    hwp->weak_ref = weak_ref_;
    auto closure = base::BindRepeating(&HWP::Run, base::Owned(hwp),
                                       std::forward<Args>(args)...);
    return BindToLoop(impl_task_runner_, std::move(closure));
  }

  T* GetRawPointerForTesting() const { return weak_ref_->ptr; }

 private:
  scoped_refptr<base::SequencedTaskRunner> impl_task_runner_;

  // Our weak ref.  One must only access the scoped_refptr from our thread, but
  // one must only dereference it on |impl_task_runner_|, since it might be
  // modified there.
  scoped_refptr<typename SequencedObject<T>::InternalWeakRef> weak_ref_;

  // Friend the SequencedScopedRefPtr for deferred pointers.
  friend class SequencedScopedRefPtr<T>;
};

}  // namespace media

#endif  // MEDIA_BASE_SEQUENCED_WEAK_PTR_H_
