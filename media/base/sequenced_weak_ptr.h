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
  SequencedWeakPtr(T& thiz) : SequencedWeakPtr(&thiz) {}
  SequencedWeakPtr(std::unique_ptr<T>& thiz) : SequencedWeakPtr(thiz.get()) {}
  SequencedWeakPtr(T* thiz)
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

  // Calls to |wp_| on some other thread.
  template <typename Method, typename... Args>
  void Post(Method method, Args... args) {
    std::move(BindOnce(method, args...)).Run(args...);
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
    scoped_refptr<typename SequencedObject<Class>::InternalWeakRef> weak_ref;

    void (Class::*method_ptr)(Args...);
    void Run(Args... args) {
      if (weak_ref->ptr)
        (weak_ref->ptr->*method_ptr)(args...);
    }
  };

  // TODO(liberato): How do we handle const-ness without duplicating this?
  template <typename Class, typename... Args>
  struct HiddenWeakPointer<void (Class::*)(Args...) const> {
    scoped_refptr<typename SequencedObject<Class>::InternalWeakRef> weak_ref;

    void (Class::*method_ptr)(Args...) const;
    void Run(Args... args) {
      if (weak_ref->ptr)
        (weak_ref->ptr->*method_ptr)(args...);
    }
  };

  template <typename... UnboundArgs>
  auto BindOnceHelper(scoped_refptr<base::SequencedTaskRunner> impl_task_runner,
                      base::OnceCallback<void(UnboundArgs...)> cb) {
    return base::BindOnce(
        [](scoped_refptr<base::SequencedTaskRunner> impl_task_runner,
           base::OnceCallback<void(UnboundArgs...)> cb,
           UnboundArgs... unbound_args) {
          impl_task_runner->PostTask(
              FROM_HERE, base::BindOnce(std::move(cb), unbound_args...));
        },
        impl_task_runner, std::move(cb));
  }

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
    auto closure = base::BindOnce(&HWP::Run, base::Owned(hwp), args...);
    return BindOnceHelper(impl_task_runner_, std::move(closure));
  }

 private:
  template <typename... UnboundArgs>
  auto BindRepeatingHelper(
      scoped_refptr<base::SequencedTaskRunner> impl_task_runner,
      base::RepeatingCallback<void(UnboundArgs...)> cb) {
    return base::BindRepeating(
        [](scoped_refptr<base::SequencedTaskRunner> impl_task_runner,
           base::RepeatingCallback<void(UnboundArgs...)> cb,
           UnboundArgs... unbound_args) {
          impl_task_runner->PostTask(
              FROM_HERE, base::BindRepeating(std::move(cb), unbound_args...));
        },
        impl_task_runner, std::move(cb));
  }

 public:
  // Create a thread-safe callback.
  template <typename Method, typename... Args>
  auto BindRepeating(Method method, Args... args) {
    // |closure| will take whatever arguments are still unbound.
    using HWP = HiddenWeakPointer<Method>;
    HWP* hwp = new HWP;
    hwp->method_ptr = method;
    hwp->weak_ref = weak_ref_;
    auto closure = base::BindRepeating(&HWP::Run, base::Owned(hwp), args...);
    return BindRepeatingHelper(impl_task_runner_, std::move(closure));
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
