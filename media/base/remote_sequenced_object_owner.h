// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_BASE_REMOTE_SEQUENCED_OBJECT_OWNER_H_
#define MEDIA_BASE_REMOTE_SEQUENCED_OBJECT_OWNER_H_

#include "base/bind.h"
#include "base/memory/ptr_util.h"
#include "base/sequenced_task_runner.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "base/unbindable_as_pointer.h"
#include "media/base/sequenced_object.h"
#include "media/base/sequenced_weak_ptr.h"

namespace media {

// Manage the lifetime of an instance of |T| on some other task runner.  |T|
// will be constructed and destroyed on an impl task runner that's provided.
//
// The idea is that, often, one runs on sequence A, but wants to create / own
// an object that runs on sequence B.  A RemoteSequencedObjectOwner lets you do
// that easily:
//
//   struct Goo : public SequencedObject<Goo> {};
//
//   struct Foo {
//     Foo(scoped_refptr<Sequenced...> goo_task_runner_) :
//       goo_(goo_task_runner_,
//            /* optional: any args for Foo(...) */ ) {}
//
//     RemoteSequencedObjectOwner<Goo> goo_owner_;
//   };
//
// Goo will be constructed on |goo_task_runner_|, and destroyed there after
// |goo_owner_| is destroyed.  You may call:
//
//   goo_owner_.GetSequencedWeakPtr()
//
// to get a SWP to talk to |goo_|.  Notice that you don't get access to a Goo*,
// so it's impossible to refer to the underlying Goo object on the wrong thread.
//
// Also note that you have no idea if the instance of Goo has been constructed
// yet, but that's okay.  It's guaranteed that construction is posted to the
// impl thread during construction of the remote owner, and the weak pointer
// will be valid by then.
//
// Remember that, from the point of view of the Goo instance, the sequence that
// on which |goo_owner_| is constructed is a remote sequence.
//
// It is legal to move a RemoteSequencedObjectOwner between threads once
// construction is complete.
template <typename T>
class RemoteSequencedObjectOwner {
 public:
  // |impl_task_runner| is the local task runner on which |t_| will live.
  template <typename... Args>
  RemoteSequencedObjectOwner(
      scoped_refptr<base::SequencedTaskRunner> impl_task_runner,
      Args... args)
      : impl_task_runner_(std::move(impl_task_runner)),
        weak_ref_(new typename SequencedObject<T>::InternalWeakRef),
        weak_factory_(this) {
    // Post construction of |t_|.
    impl_task_runner_->PostTask(
        FROM_HERE,
        base::BindOnce(
            &RemoteSequencedObjectOwner<T>::ConstructOnImplSequence<Args...>,
            base::SequencedTaskRunnerHandle::Get(), weak_ref_,
            impl_task_runner_, weak_factory_.GetWeakPtr(), args...));
  }

  // If you've created an object that should be owned on the current thread, you
  // may assign ownership to us.  This is useful to transfer ownership to some
  // other object.
  RemoteSequencedObjectOwner(std::unique_ptr<T> t)
      : t_(std::move(t)),
        impl_task_runner_(base::SequencedTaskRunnerHandle::Get()),
        weak_factory_(this) {
    // We don't need to create our own weak ref, since |t| already has
    // one.  Just use that one.
    weak_ref_ = t_->GetWeakRef();
  }

  ~RemoteSequencedObjectOwner() {
    // Post destruction of |t_|.
    // It would be nice if we couldn't DeleteSoon a SequencedObject, but
    // it seems to evade our detection.  So, for now, we skip wrapping
    // this in a HiddenUniquePointer.
    impl_task_runner_->DeleteSoon(FROM_HERE, std::move(t_));
  }

  SequencedWeakPtr<T> GetSequencedWeakPtr() {
    DCHECK(t_);
    // Note that |weak_ref_| might not be filled in yet, but that's okay.  It
    // will be filled in before it can be used on |impl_task_runner_|, since we
    // already posted construction there.
    return SequencedWeakPtr<T>(impl_task_runner_, weak_ref_);
  }

 private:
  // Let's just say we'd like to avoid any... base::Bind entanglements.
  struct HiddenUniquePointer {
    std::unique_ptr<T> t;
  };

  // Run on the impl sequence to actually construct |t_|, and return it to us
  // via |wp| on the orignal thread.
  template <typename... Args>
  static void ConstructOnImplSequence(
      scoped_refptr<base::SequencedTaskRunner> owner_task_runner,
      scoped_refptr<typename SequencedObject<T>::InternalWeakRef> weak_ref,
      scoped_refptr<base::SequencedTaskRunner> impl_task_runner,
      base::WeakPtr<RemoteSequencedObjectOwner<T>> wp,
      Args... args) {
    HiddenUniquePointer ptr;
    ptr.t = base::MakeUnique<T>(args...);

    // Fill in |weak_ref| for any refs that we've already sent out.
    weak_ref->ptr = ptr.t.get();

    // Register |weak_ref_| so that |ptr.t| clears it on destruction.
    ptr.t->RegisterWeakRef(weak_ref.get());

    // Post |ptr| back to the owner's thread.  Note that OnContructionComplete
    // is static, so it will always run.  It will check |wp| and destruct |ptr|
    // on |impl_task_runner| if the remote object owner has been destroyed.
    owner_task_runner->PostTask(
        FROM_HERE,
        base::BindOnce(&RemoteSequencedObjectOwner<T>::OnConstructionComplete,
                       wp, std::move(impl_task_runner), std::move(ptr)));
  }

  // This is run on the local sequence to receive the unique ptr.  If |wp| has
  // been deleted already, then post |ptr| back to |impl_task_runner| for
  // destruction.  This happens if the owner is deleted while construction is
  // in progress on the impl thread.
  static void OnConstructionComplete(
      base::WeakPtr<RemoteSequencedObjectOwner<T>> wp,
      scoped_refptr<base::SequencedTaskRunner> impl_task_runner,
      HiddenUniquePointer ptr) {
    if (wp)
      wp->t_ = std::move(ptr.t);
    else
      impl_task_runner->DeleteSoon(std::move(ptr.t));
  }

  // The unique_ptr should only be accessed from our local thread.  The object
  // it points to, however, must only be accessed from |impl_task_runner_|.
  std::unique_ptr<T> t_;

  // Task runner on which |t_| lives.
  scoped_refptr<base::SequencedTaskRunner> impl_task_runner_;

  // Weak ref to |t_|.  It is created here, but its pointer is filled in later.
  scoped_refptr<typename SequencedObject<T>::InternalWeakRef> weak_ref_;

  base::WeakPtrFactory<RemoteSequencedObjectOwner<T>> weak_factory_;
};

}  // namespace media

#endif  // MEDIA_BASE_REMOTE_SEQUENCED_OBJECT_OWNER_H_
