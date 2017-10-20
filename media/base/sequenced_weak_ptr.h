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
class RemoteSequencedObjectOwner;

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

  // These must be called from the home sequence.  However, since nothing
  // but the home sequence should have |T|, that's fine.
  SequencedWeakPtr(T& thiz) : SequencedWeakPtr(&thiz) {}
  SequencedWeakPtr(std::unique_ptr<T>& thiz) : SequencedWeakPtr(thiz.get()) {}
  SequencedWeakPtr(T* thiz)
      : task_runner_(static_cast<SequencedObject<T>*>(thiz)->task_runner_),
        weak_ref_(thiz->GetWeakRef()) {}

  // This one must be thread-safe and may be called from anywhere.
  SequencedWeakPtr(const SequencedWeakPtr& other)
      : weak_ref_(other.weak_ref_) {}

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

  // Utility class to hold a wp that we don't have yet.
  // TODO(liberato): could we do this better with in-place construction of an
  // object?  As in, we can allocate it in RemoteSequencedObjectOwner's
  // constructor, then in-place construct it on the right thread.  If WeakPtr
  // worked slightly differently (had the ref inside Flag), then we could just
  // pass around the Flag, and there would be no need of "deferred" pointers.
  // Since we don't really need WPs, we probably should just use our own
  // internal implementation that does that.  that would also let us remove the
  // "HiddenWeakPointer" class, since this would do the same thing.
  struct DeferredPointer : public base::RefCountedThreadSafe<DeferredPointer> {
    base::WeakPtr<T> wp;
  };

  // Private constructor for deferred SWPs.  This is useful when we know that a
  // wp will exist on |task_runner|, but we don't know what it is yet.  Our
  // caller should provide the deferred struct, and guarantee that it is filled
  // in with a valid weak ptr before we're used.
  SequencedWeakPtr(
      scoped_refptr<base::SequencedTaskRunner> task_runner,
      scoped_refptr<typename SequencedObject<T>::InternalWeakRef> weak_ref)
      : task_runner_(std::move(task_runner)), weak_ref_(std::move(weak_ref)) {}

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
  auto BindOnceHelper(scoped_refptr<base::SequencedTaskRunner> task_runner,
                      base::OnceCallback<void(UnboundArgs...)> cb) {
    return base::BindOnce(
        [](scoped_refptr<base::SequencedTaskRunner> task_runner,
           base::OnceCallback<void(UnboundArgs...)> cb,
           UnboundArgs... unbound_args) {
          task_runner->PostTask(FROM_HERE,
                                base::BindOnce(std::move(cb), unbound_args...));
        },
        task_runner, std::move(cb));
  }

 public:
  // Create a thread-safe callback.  It would be nice if Bind did this
  // automatically if given a SequencedWeakPtr.
  template <typename Method, typename... Args>
  auto BindOnce(Method method, Args... args) {
    // |closure| will take whatever arguments are still unbound.
    // We have to hide |wp_| since it can't be bound directly.
    using HWP = HiddenWeakPointer<Method>;
    HWP* hwp = new HWP;
    hwp->method_ptr = method;
    hwp->weak_ref = weak_ref_;
    auto closure = base::BindOnce(&HWP::Run, base::Owned(hwp), args...);
    return BindOnceHelper(task_runner_, std::move(closure));
  }

 private:
  template <typename... UnboundArgs>
  auto BindRepeatingHelper(scoped_refptr<base::SequencedTaskRunner> task_runner,
                           base::RepeatingCallback<void(UnboundArgs...)> cb) {
    return base::BindRepeating(
        [](scoped_refptr<base::SequencedTaskRunner> task_runner,
           base::RepeatingCallback<void(UnboundArgs...)> cb,
           UnboundArgs... unbound_args) {
          task_runner->PostTask(
              FROM_HERE, base::BindRepeating(std::move(cb), unbound_args...));
        },
        task_runner, std::move(cb));
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
    return BindRepeatingHelper(task_runner_, std::move(closure));
  }

 private:
  scoped_refptr<base::SequencedTaskRunner> task_runner_;

  scoped_refptr<typename SequencedObject<T>::InternalWeakRef> weak_ref_;

  // Friend the RemoteSequencedObjectOwner for deferred pointers.
  friend class RemoteSequencedObjectOwner<T>;
};

}  // namespace media

#endif  // MEDIA_BASE_SEQUENCED_WEAK_PTR_H_
