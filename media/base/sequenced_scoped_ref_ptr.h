// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_BASE_SEQUENCD_SCOPED_REF_PTR_H_
#define MEDIA_BASE_SEQUENCD_SCOPED_REF_PTR_H_

#include "base/memory/ptr_util.h"
#include "base/sequenced_task_runner.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "media/base/sequenced_object.h"
#include "media/base/sequenced_weak_ptr.h"

namespace media {

namespace internal {
template <typename T, typename Impl, typename... Args>
static std::unique_ptr<T> SequencedFactory(Args... args) {
  return base::MakeUnique<Impl>(std::move(args)...);
}
}  // namespace internal

// Manage the lifetime of an instance of |T| on some other task runner.  |T|
// will be constructed and destroyed on an impl task runner that's provided once
// all references are dropped.
//
// The idea is that, often, one runs on sequence A, but wants to create / own
// an object that runs on sequence B.  A SequencedScopedRefPtr lets you do
// that easily:
//
//   struct Goo : public SequencedObject<Goo> {};
//
//   struct Foo {
//     Foo(scoped_refptr<Sequenced...> goo_task_runner_) :
//       goo_(goo_task_runner_,
//            /* optional: any args for Foo(...) */ ) {}
//
//     SequencedScopedRefPtr<Goo> goo_owner_;
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
// It is legal to copy and/or move a SequencedScopedRefPtr between threads once
// construction is complete.  The underlying object will be retained until the
// last copy is destroyed.
//
// TODO(liberato): I don't like this name.  Maybe SequencedObjectSharedOwner?
// SequencedObjectSharedRef?  SequencedScopedRefPtr?
template <typename T>
class SequencedScopedRefPtr {
 public:
  template <typename Impl, typename... Args>
  static SequencedScopedRefPtr<T> Create(
      scoped_refptr<base::SequencedTaskRunner> impl_task_runner,
      Args... args) {
    return SequencedScopedRefPtr<T>(
        impl_task_runner,
        base::BindOnce(&internal::SequencedFactory<T, Impl, Args...>,
                       std::move(args)...));
  };

  // Unbound refptr.  To use it, assign or assignment-move a refptr here.
  // If you later construct a new object, then you may use a temporary:
  // SequencedScopedRefPtr x;
  // x = SequencedScopedRefPtr(task_runner_, ...);
  // x = SeuencedScopedRefPtr(base::MakeUnique(...));
  SequencedScopedRefPtr() {}

  template <typename... Args>
  SequencedScopedRefPtr(
      scoped_refptr<base::SequencedTaskRunner> impl_task_runner,
      Args... args)
      : SequencedScopedRefPtr<T>(
            std::move(impl_task_runner),
            base::BindOnce(&internal::SequencedFactory<T, T, Args...>,
                           std::move(args)...)) {}

 protected:
  // |impl_task_runner| is the local task runner on which |t_| will live.
  SequencedScopedRefPtr(
      scoped_refptr<base::SequencedTaskRunner> impl_task_runner,
      base::OnceCallback<std::unique_ptr<T>()> factory_cb)
      : record_(new OwnerRecord(base::SequencedTaskRunnerHandle::Get())),
        weak_ref_(new typename SequencedObject<T>::InternalWeakRef) {
    // Post construction of |t_|.
    record_->impl_task_runner->PostTask(
        FROM_HERE,
        base::BindOnce(&SequencedScopedRefPtr<T>::ConstructOnImplSequence,
                       std::move(factory_cb), record_, weak_ref_));
  }

 public:
  // If you've created an object that should be owned on the current thread, you
  // may assign ownership to us.  This is useful to transfer ownership to some
  // other object.
  SequencedScopedRefPtr(std::unique_ptr<T> t)
      : record_(new OwnerRecord(std::move(t),
                                base::SequencedTaskRunnerHandle::Get())) {
    // We don't need to create our own weak ref, since |t| already has
    // one.  Just use that one.  Remember, we're on the right thread else this
    // constructor shouldn't be used.
    weak_ref_ = record_->t->GetWeakRef();
  }

  SequencedScopedRefPtr(const SequencedScopedRefPtr& other)
      : record_(other.record_), weak_ref_(other.weak_ref_) {}

  SequencedScopedRefPtr(SequencedScopedRefPtr&& other)
      : record_(std::move(other.record_)),
        weak_ref_(std::move(other.weak_ref_)) {}

  SequencedScopedRefPtr& operator=(const SequencedScopedRefPtr& other) {
    record_ = other.record_;
    weak_ref_ = other.weak_ref_;
    return *this;
  }

  SequencedScopedRefPtr& operator=(SequencedScopedRefPtr&& other) {
    record_ = std::move(other.record_);
    weak_ref_ = std::move(other.weak_ref_);
    return *this;
  }

  ~SequencedScopedRefPtr() {}

  SequencedWeakPtr<T> GetSequencedWeakPtr() const {
    // Note that |weak_ref_| might not be filled in yet, but that's okay.  It
    // will be filled in before it can be used on |impl_task_runner|, since we
    // already posted construction there.
    return SequencedWeakPtr<T>(record_->impl_task_runner, weak_ref_);
  }

  // Drop our reference.
  void reset() {
    record_ = nullptr;
    weak_ref_ = nullptr;
  }

  // Return whether we're bound to an object or not.  This does not guarantee
  // that the object has been constructed yet.  It only guarantees that it will
  // be eventually assuming that the reference is held long enough.
  explicit operator bool() const { return record_ != nullptr; }

  T* GetRawPointerForTesting() const { return record_->t.get(); }

 private:
  // This owns the remote object, and guarantees that it will be destructed on
  // the proper thread.
  struct OwnerRecord : public base::RefCountedThreadSafe<OwnerRecord> {
    OwnerRecord(scoped_refptr<base::SequencedTaskRunner> impl)
        : impl_task_runner(std::move(impl)) {}

    OwnerRecord(std::unique_ptr<T> t0,
                scoped_refptr<base::SequencedTaskRunner> impl)
        : t(std::move(t0)), impl_task_runner(std::move(impl)) {}

    virtual ~OwnerRecord() {
      // Post destruction of |t_|.
      // It would be nice if we couldn't DeleteSoon a SequencedObject, but
      // it seems to evade our detection.  So, for now, we skip wrapping this in
      // something that would hide it.
      if (t)
        impl_task_runner->DeleteSoon(FROM_HERE, std::move(t));
    }

    // |t| may be modified only on the impl thread with a strong ref to |this|
    // held.  It may also be modified from any thread from our destructor.
    // Of course, dereferencing |t| is only allowed on the impl sequence.
    std::unique_ptr<T> t;

    // This is set during construction, and may be read from any thread.
    const scoped_refptr<base::SequencedTaskRunner> impl_task_runner;

    // NOTE: we could keep weak_ref here too, but it doesn't matter since it's
    // already a scoped_refptr.
  };

  // Run on the impl sequence to actually construct |t_|, and return it to us
  // via |wp| on the orignal thread.
  // TODO(liberato): consider moving |weak_ref| into |record| and moving this
  // whole function into OwnerRecord.
  static void ConstructOnImplSequence(
      base::OnceCallback<std::unique_ptr<T>()> factory_cb,
      scoped_refptr<OwnerRecord> record,
      scoped_refptr<typename SequencedObject<T>::InternalWeakRef> weak_ref) {
    // It is safe to do this, since only the impl thread may access |t|.
    // One exception is the destuction of |record|, but since we hold a ref to
    // it, that can't happen.
    record->t = std::move(factory_cb).Run();

    // Fill in |weak_ref| for any refs that we've already sent out.
    weak_ref->ptr = record->t.get();

    // Register |weak_ref_| so that |ptr.t| clears it on destruction.
    record->t->RegisterWeakRef(weak_ref.get());
  }

  // Our reference to the shared owner.
  scoped_refptr<OwnerRecord> record_;

  // Weak ref to |t_|.  It is created here, but its pointer is filled in later.
  scoped_refptr<typename SequencedObject<T>::InternalWeakRef> weak_ref_;
};

}  // namespace media

#endif  // MEDIA_BASE_SEQUENCD_SCOPED_REF_PTR_H_
