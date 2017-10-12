// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_BASE_SEQUENCED_OBJECT_H_
#define MEDIA_BASE_SEQUENCED_OBJECT_H_

#include "base/bind.h"
#include "base/memory/ptr_util.h"
#include "base/memory/weak_ptr.h"
#include "base/sequenced_task_runner.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "base/unbindable_as_pointer.h"

namespace media {

template <typename T>
class SequencedWeakPointer;

// A SequencedObject is an object that lives entirely on one sequence.  It
// cannot (accidentally) be passed between threads via callback, since
// raw / unique / weak pointers may not be included in the signature of any
// bound methods.  This restricts access to the object to its owning sequence.
//
// For example, the following will all fail to compile:
//
//   struct Foo : SequencedObject<Foo> { ... };
//   WeakPointer<Foo> wp;
//   base::BindOnce(&Foo:AnyMethod, &foo)  // Will not compile.
//   base::BindOnce(&Foo:AnyMethod, wp)  // Will not compile.
//
// What one may do, however, is use a SequencedWeakPointer for callbacks.  This
// will guarantee that the callback is run on the owning sequence, no matter
// where its Run() method is called from:
//
//   SequencedWeakPointer<Foo> swp(foo);
//   base::BindOnce(&Foo::AnyMethod, swp);  // Will work fine.
//   // May post elsewhere and run, will post back to foo's owning sequence.
//
// Calling Run() on this callback will post to the thread on which |foo| lives.
// TODO(liberato): What's the preferred behavior if we're on that thread
// already?  Currently, it will still post, so that it's always async.
//
// If you want to send |foo| to another thread, it is also legal to bind a
// SequencedWeakPointer as an argument:
//
//   base::BindOnce(&AnyFunction, ... , swp);
//
// SequencedWeakPointers may be copied / moved by the receiver if desired.  The
// receiver may use them to generate new callbacks for |foo|, which will still
// post to the appropriate thread.
//
// In other words, the remote thread never gets the opportunity to post |foo| to
// the wrong thread, call methods directly on |foo| from the wrong thread, etc.
template <typename T>
class SequencedObject : public base::UnbindableAsPointer {
 public:
  SequencedObject() : SequencedObject(base::SequencedTaskRunnerHandle::Get()) {}

  // Note that |this| isn't T-constructed yet, but that's okay.
  SequencedObject(scoped_refptr<base::SequencedTaskRunner> task_runner)
      : task_runner_(std::move(task_runner)),
        weak_factory_(static_cast<T*>(this)) {}

  // TODO(liberato): include BindOnce / BindRepeating helpers.

 private:
  friend class SequencedWeakPointer<T>;

  // Note: we should allow subclasses to override the function that returns
  // weak ptrs, so that the subclass can control when they are invalidated
  // if it wants.  Or maybe, just add "InvalidateWeakPointers()".
  scoped_refptr<base::SequencedTaskRunner> task_runner_;
  base::WeakPtrFactory<T> weak_factory_;
};

// A SequencedObjectWrapper is a utility class to wrap existing objects up as
// SequencedObjects.  For new classes, derive from SequencedObject directly.
template <typename T>
class SequencedObjectWrapper
    : public SequencedObject<SequencedObjectWrapper<T>>,
      public T {
 public:
  // Construct |T|
  template <typename... Args>
  SequencedObjectWrapper(Args... args) : T(args...) {}
};

// A SequencedWeakPointer is the only way for something that does't own the
// object to call methods on it.  All method invocations will be run
// asynchronously, on the correct sequence for the SequencedObject.
template <typename T>
class SequencedWeakPointer {
 public:
  SequencedWeakPointer() {}

  // These must be called from the home sequence.  However, since nothing
  // but the home sequence should have |T|, that's fine.
  SequencedWeakPointer(T& thiz) : SequencedWeakPointer(&thiz) {}
  SequencedWeakPointer(std::unique_ptr<T>& thiz)
      : SequencedWeakPointer(thiz.get()) {}
  SequencedWeakPointer(T* thiz)
      : task_runner_(static_cast<SequencedObject<T>*>(thiz)->task_runner_),
        wp_(static_cast<SequencedObject<T>*>(thiz)
                ->weak_factory_.GetWeakPtr()) {}

  // This one must be thread-safe and may be called from anywhere.
  SequencedWeakPointer(const SequencedWeakPointer& other) : wp_(other.wp_) {}

  // Calls to |wp_| on some other thread.
  template <typename Method, typename... Args>
  void Post(Method method, Args... args) {
    std::move(BindOnce(method, args...)).Run(args...);
  }

 private:
  // Helper class to hide a WeakPtr<UnbindableAsPointer> type, since we can't
  // bind it directly.  The Run method will have the same signature as |Method|,
  // but will forward the call to the original method via |wp|.
  template <typename Method>
  struct HiddenWeakPointer;
  template <typename Class, typename... Args>
  struct HiddenWeakPointer<void (Class::*)(Args...)> {
    base::WeakPtr<Class> wp;
    void (Class::*method_ptr)(Args...);
    void Run(Args... args) {
      if (wp)
        ((wp.get())->*method_ptr)(args...);
    }
  };
  // TODO(liberato): How do we handle const-ness without duplicating this?
  template <typename Class, typename... Args>
  struct HiddenWeakPointer<void (Class::*)(Args...) const> {
    base::WeakPtr<Class> wp;
    void (Class::*method_ptr)(Args...) const;
    void Run(Args... args) {
      if (wp)
        ((wp.get())->*method_ptr)(args...);
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
  // automatically if given a SequencedWeakPointer.
  template <typename Method, typename... Args>
  auto BindOnce(Method method, Args... args) {
    // |closure| will take whatever arguments are still unbound.
    // We have to hide |wp_| since it can't be bound directly.
    using HWP = HiddenWeakPointer<Method>;
    HWP* hwp = new HWP;
    hwp->method_ptr = method;
    hwp->wp = wp_;
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
    hwp->wp = wp_;
    auto closure = base::BindRepeating(&HWP::Run, base::Owned(hwp), args...);
    return BindRepeatingHelper(task_runner_, std::move(closure));
  }

 private:
  scoped_refptr<base::SequencedTaskRunner> task_runner_;
  base::WeakPtr<T> wp_;
};

// Manage the lifetime of an instance of |T| on some other task runner.  |T|
// will be constructed and destroyed on the remote task runner.
//
// The idea is that, often, one runs on sequence A, but wants to create / own
// an object that runs on sequence B.  A RemoteSequencedObjectOwner lets you do
// that easily:
//
//   struct Goo : public SequencedObject<Goo> {};
//   struct Foo {
//     Foo(scoped_refptr<Sequenced...> goo_task_runner_) :
//       goo_(goo_task_runner_,
//            base::BindOnce(&Foo::OnGooConstructed, base::Unretained(this))) {}
//
//     RemoteSequencedObjectOwner<Goo> goo_;
//   };
//
// It's safe to base::Unretained(this), since |goo_| is a member variable.  If
// it weren't, then you should use a WeakPtr (or a callback made by
// SequencedWeakPointer::BindOnce, if Foo is, itself, a SequencedObject).
//
// Goo will be constructed on |goo_task_runner_|, and destroyed there after
// |goo_| is destroyed.  In the interim, one may goo_.GetSequencedWeakPointer()
// to get a SWP to talk to |goo_|.  Notice that you don't get access to a Goo*,
// so it's impossible to refer to the underlying Goo object on the wrong thread.
//
// TODO(liberato): we are a 'remote' thread.  |remote_task_runner_| should be
// called the owning task runner, since it owns |t_|.
template <typename T>
class RemoteSequencedObjectOwner {
 public:
  // |task_runner| is the remote task runner on which |t_| will live.
  template <typename... Args>
  RemoteSequencedObjectOwner(
      scoped_refptr<base::SequencedTaskRunner> task_runner,
      base::OnceClosure on_constructed_cb,
      Args... args)
      : remote_task_runner_(std::move(task_runner)),
        on_constructed_cb_(std::move(on_constructed_cb)),
        weak_factory_(this) {
    // Post construction of |t_|.
    remote_task_runner_->PostTask(
        FROM_HERE,
        base::BindOnce(
            &RemoteSequencedObjectOwner<T>::ConstructOnRemoteThread<Args...>,
            base::SequencedTaskRunnerHandle::Get(), weak_factory_.GetWeakPtr(),
            args...));
  }

  ~RemoteSequencedObjectOwner() {
    // Post destruction of |t_|.
    // It would be nice if we couldn't DeleteSoon a SequencedObject, but
    // it seems to evade our detection.  So, for now, we skip wrapping
    // this in a HiddenUniquePointer.
    remote_task_runner_->DeleteSoon(FROM_HERE, std::move(t_));
  }

  SequencedWeakPointer<T> GetSequencedWeakPointer() {
    DCHECK(t_);
    return swp_;
  }

 private:
  // Let's just say we'd like to avoid any... base::Bind entanglements.
  struct HiddenUniquePointer {
    std::unique_ptr<T> t;
  };

  // Run on a remote sequence to actually construct |t_|, and return it
  // to us via |wp| on the owning thread.
  template <typename... Args>
  static void ConstructOnRemoteThread(
      scoped_refptr<base::SequencedTaskRunner> owning_task_runner,
      base::WeakPtr<RemoteSequencedObjectOwner<T>> wp,
      Args... args) {
    HiddenUniquePointer ptr;
    ptr.t = base::MakeUnique<T>(args...);
    SequencedWeakPointer<T> swp(ptr.t);
    owning_task_runner->PostTask(
        FROM_HERE,
        base::BindOnce(&RemoteSequencedObjectOwner<T>::OnConstructionComplete,
                       wp, std::move(ptr), swp));
  }

  // Run on the local (owner) sequence to receive the unique ptr, and
  // also a SequencedWeakPointer.  We can't actually construct one here,
  // since that requries accessing the remote object.  We can only copy
  // existing ones.
  // Note that if the remote object invalidates its weak pointers, we
  // can't get a new one, unfortunately.
  void OnConstructionComplete(HiddenUniquePointer ptr,
                              SequencedWeakPointer<T> swp) {
    t_ = std::move(ptr.t);
    swp_ = swp;
    if (on_constructed_cb_)
      std::move(on_constructed_cb_).Run();
  }

  std::unique_ptr<T> t_;
  SequencedWeakPointer<T> swp_;
  scoped_refptr<base::SequencedTaskRunner> remote_task_runner_;
  base::OnceClosure on_constructed_cb_;
  base::WeakPtrFactory<RemoteSequencedObjectOwner<T>> weak_factory_;
};

}  // namespace media

#endif  // MEDIA_BASE_SEQUENCED_OBJECT_H_
