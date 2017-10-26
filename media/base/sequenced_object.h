// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_BASE_SEQUENCED_OBJECT_H_
#define MEDIA_BASE_SEQUENCED_OBJECT_H_

#include <vector>

#include "base/bind.h"
#include "base/memory/ptr_util.h"
#include "base/sequenced_task_runner.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "base/unbindable_as_pointer.h"

namespace media {

template <typename T>
class SequencedWeakPtr;

template <typename T>
class SequencedScopedRefPtr;

// A SequencedObject is an object that lives entirely on one sequence.  The
// main idea is that a SequencedObject:
//
//  - is created, used, and destroyed from a single sequence ("impl sequence").
//  - is not easily passed to other sequences accidentally.
//  - supports "sequence hopping" callbacks easily.
//
// It cannot (accidentally) be passed between threads via callback, since
// raw / unique / weak pointers may not be included in the signature of any
// bound methods.  This restricts access to the object to its impl sequence,
// at least in many common cases.
//
// For example, the following will all fail to compile:
//
//   struct Foo : SequencedObject<Foo> {
//     void AnyMethod(int) { ... }
//   };
//   WeakPointer<Foo> wp;
//   base::BindOnce(&Foo:AnyMethod, &foo)  // Will not compile.
//   base::BindOnce(&Foo:AnyMethod, wp)  // Will not compile.
//
// What one may do, however, is use a SequencedWeakPtr for callbacks.  This
// will guarantee that the callback is run on the impl sequence, no matter
// where its Run() method is called from:
//
//   SequencedWeakPtr<Foo> swp(foo);
//   swp.BindOnce(&Foo::AnyMethod, 123);  // Will work fine.
//   // May post elsewhere and run, will post back to foo's impl sequence.
//
// Calling Run() on this callback will post to the thread on which |foo| lives.
//
// TODO(liberato): "and reply".
//
// If you want to send |foo| to another thread, it is also legal to bind a
// SequencedWeakPtr as a function / method argument:
//
//   void AnyFunction(SequencedWeakPtr<Foo> swp) { ... }
//   base::BindOnce(&AnyFunction, swp);  // Will work fine.
//
// SequencedWeakPtrs may be copied / moved by the receiver if desired.  The
// receiver may use them to generate new callbacks for |foo|, which will still
// post to the appropriate thread.
//
// In other words, the remote thread never gets the opportunity to post |foo| to
// the wrong thread, call methods directly on |foo| from the wrong thread, etc.
//
// If you'd like to create a SequencedObject that lives on a different sequence
// from yours, please see SequencedScopedRefPtr and SequencedUniquePtr.
//
// Side note about swp.Bind methods:
//
// It's possible to make base::Bind(&Foo:method, swp, ...) automatically create
// a thread-hopping callback.  It's not terribly difficult to do, as long as one
// binds the method and the swp at the same time (e.g., Bind(&Foo::method).Run(
// swp, ...) is hard to fix.  maybe we need to override Run instead.).
//
// Do we want to do that, or is it too subtle?  Since one cannot easily create
// a non-thread-hopping callback via base::Bind (i.e. sending in a raw ptr or
// otherwise won't compile; they're already sending in a SWP), it seems like
// it's not too unexpected.  The only weird thing is that Run() becomes async,
// when normally Bind(...).Run() is not.
template <typename T>
class SequencedObject : public base::UnbindableAsPointer {
 public:
  // So derived classes can find us.
  using SequencedObjectBaseType = T;

  SequencedObject() : task_runner_(base::SequencedTaskRunnerHandle::Get()) {
    RegisterWeakRef(new InternalWeakRef(static_cast<T*>(this)));
  }

  // Note that |this| isn't T-constructed yet, but that's okay.
  SequencedObject(scoped_refptr<base::SequencedTaskRunner> task_runner)
      : task_runner_(std::move(task_runner)) {
    RegisterWeakRef(new InternalWeakRef(static_cast<T*>(this)));
  }

  ~SequencedObject() { ClearWeakRefs(); }

  // TODO(liberato): include BindOnce / BindRepeating helpers.  This helps the
  // subclass bind callbacks of its own methods without having to do this:
  // SequencedWeakPtr<Foo>(this).BindOnce(&Foo::Method, ...);
  template <typename Functor, typename... Args>
  auto BindOnce(Functor functor, Args... args) {
    SequencedWeakPtr<T> swp(static_cast<T*>(this));
    return swp.BindOnce(functor, std::forward<Args>(args)...);
  }

  template <typename Functor, typename... Args>
  auto BindRepeating(Functor functor, Args... args) {
    SequencedWeakPtr<T> swp(static_cast<T*>(this));
    return swp.BindRepeating(functor, std::forward<Args>(args)...);
  }

 private:
  friend class SequencedWeakPtr<T>;
  friend class SequencedScopedRefPtr<T>;

  scoped_refptr<base::SequencedTaskRunner> task_runner_;

  // We don't re-use base::WeakPtr here, since we need to issue weak refs before
  // we're constructed.  WeakPtr almost lets us do that, but not quite.  The
  // use-case is that SequencedScopedRefPtr might want to hand out ptrs
  // before we've been constructed, but it knows that they can't be used except
  // on the construction thread.  So, it's safe.  It's possible to do this with
  // base::WeakPtr + a lot of hoops, but it's much easier just to memorize this
  // functionality we need.
  struct InternalWeakRef : public base::RefCountedThreadSafe<InternalWeakRef> {
    InternalWeakRef() : ptr(nullptr) {}
    InternalWeakRef(T* p) : ptr(p) {}
    T* ptr;
  };

  // Register a new weak ref.
  // We only need to support more than one, in case we're remotely constructed
  // and the object's constructor creates weak refs for itself.  If we could
  // send the remotely-constructed weakref in during construction, then we could
  // just use one.
  void RegisterWeakRef(InternalWeakRef* weak_ref) {
    weak_refs_.push_back(weak_ref);
  }

  // Get any weak ref.
  InternalWeakRef* GetWeakRef() { return weak_refs_[0].get(); }

  void ClearWeakRefs() {
    for (auto& ref : weak_refs_) {
      ref->ptr = nullptr;
    }
    weak_refs_.clear();
  }

  // Zero, one, or two weak refs.
  std::vector<scoped_refptr<InternalWeakRef>> weak_refs_;
};

// A SequencedObjectWrapper is a utility class to wrap existing objects up as
// SequencedObjects.  For new classes, derive from SequencedObject directly.
//
// If you want to expose an existing object as a SequencedObject, then wrap it
// in a SequencedObjectWrapper.  Be aware, however, that the original object
// obviously won't derive from SequencedObject, so none of the base::Bind
// protections will notice if the base class base::Bind()s directly to itself.
//
// Also be careful if one passes the wrapper as T, since it derives from t.
//
// TODO(liberato): this class seems much too easy to use incorrectly.
template <typename T>
class SequencedObjectWrapper
    : public SequencedObject<SequencedObjectWrapper<T>>,
      public T {
 public:
  // Construct |T|
  template <typename... Args>
  SequencedObjectWrapper(Args... args) : T(args...) {}
};

}  // namespace media

#endif  // MEDIA_BASE_SEQUENCED_OBJECT_H_
