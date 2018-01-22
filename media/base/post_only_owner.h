// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_BASE_POST_ONLY_OWNER_H_
#define MEDIA_BASE_POST_ONLY_OWNER_H_

#include "base/bind.h"
#include "base/callback.h"
#include "base/location.h"
#include "base/memory/ptr_util.h"
#include "base/sequenced_task_runner.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "media/base/bind_to_current_loop.h"
#include "media/base/media_export.h"

namespace media {

// Owner for |C|, which should run on some fixed sequence.  This class will
// handle construction, method calls (posts), and destruction on that sequence.
//
// One may move-construct and move-assign these things, though standard rules
// of casting apply.  In particular, you probably want to have virtual
// destructors when you do that.  We'll always free the memory, though.
template <typename C>
class PostOnlyOwner {
 protected:
  // Helper class to make a protected constructor available.  This makes it easy
  // to hide constructors, to prevent calling new directly accidentally.
  class ConstructionHelper : public C {
   public:
    template <typename... Args>
    ConstructionHelper(Args... args) : C(std::move(args)...) {}
  };

 public:
  // Allow empty owners.
  PostOnlyOwner() {}

  // Construct a new instance of |C| that will be accessed only on
  // |task_runner|.  One may post calls to it immediately upon return.
  template <typename... Args>
  PostOnlyOwner(scoped_refptr<base::SequencedTaskRunner> task_runner,
                Args... args)
      : task_runner_(std::move(task_runner)) {
    // We do not use new() since we do not want to construct it here.  We do not
    // post non-placement new to |task_runner| since we want to have the address
    // of the object on this thread immediately.  With some extra work, we could
    // post this with new via a shared holder object, but that adds template
    // overhead without really much benefit; one needs a common base to type-
    // erase the holder as far as I can tell.
    ConstructionHelper* helper = reinterpret_cast<ConstructionHelper*>(
        malloc(sizeof(ConstructionHelper)));
    ptr_to_free_ = static_cast<void*>(helper);
    ptr_ = static_cast<C*>(helper);

    // Post construction to |task_runner_|.
    task_runner_->PostTask(FROM_HERE,
                           base::BindOnce(
                               [](ConstructionHelper* c, Args... args) {
                                 new (c) ConstructionHelper(std::move(args)...);
                               },
                               base::Unretained(helper), std::move(args)...));
  }

  ~PostOnlyOwner() { reset(); }

  // Move construction is supported from any type that's compatible with |C|.
  // Note that this is used by U==T, since there is no implicitly defined move
  // constructor due to the user-provided destructor (at least).
  template <typename U>
  PostOnlyOwner(PostOnlyOwner<U>&& other)
      : ptr_(other.ptr_),
        ptr_to_free_(other.ptr_to_free_),
        task_runner_(std::move(other.task_runner_)) {
    other.ptr_ = nullptr;
    other.ptr_to_free_ = nullptr;
  }

  // Move assignment is supported for any type that's compatible with |C|.
  // We could also provide something like static_cast<> for downcasting.
  // Note that this is used for U==T, because of the user-supplied destructor.
  // We have to be careful here, since the call to reset() wouldn't be present
  // in any default impl.  The same is true, i think, for clearing |ptr_| etc.
  template <typename U>
  PostOnlyOwner<C>& operator=(PostOnlyOwner<U>&& other) {
    if (other.is_null()) {
      reset();
    } else {
      ptr_ = other.ptr_;
      other.ptr_ = nullptr;
      ptr_to_free_ = other.ptr_to_free_;
      other.ptr_to_free_ = nullptr;
      task_runner_ = std::move(other.task_runner_);
    }
    return *this;
  }

  // Post a call to |member| to |task_runner_|, and ignore any result.
  template <typename MemberPtr, typename... Args>
  void Post(const base::Location& from_here,
            MemberPtr member,
            Args... args) const {
    task_runner_->PostTask(
        from_here,
        base::BindOnce([](MemberPtr member, C* c,
                          Args... args) { (c->*member)(std::move(args)...); },
                       member, base::Unretained(ptr_), std::move(args)...));
  }

  // Post a call to |member|, and provide it s result to |return_cb| on the
  // current sequence.
  template <typename MemberPtr, typename Return, typename... Args>
  void PostAndReply(const base::Location& from_here,
                    base::OnceCallback<void(Return)> return_cb,
                    MemberPtr member,
                    Args... args) const {
    auto bound_cb = BindToCurrentLoop(std::move(return_cb));
    task_runner_->PostTask(
        from_here,
        base::BindOnce(
            [](MemberPtr member, C* c,
               base::OnceCallback<void(Return)> return_cb, Args... args) {
              std::move(return_cb).Run((c->*member)(std::move(args)...));
            },
            member, base::Unretained(ptr_), std::move(bound_cb),
            std::move(args)...));
  }

  // Post destruction of any object we own, and return to the null state.
  void reset() {
    if (is_null())
      return;

    // Destruct |ptr_|, but free whatever the original pointer was.  Note that
    // |C| might not actually be the original type, but that's okay.  It's up
    // to the user to follow the usual rules of casting.
    task_runner_->PostTask(FROM_HERE, base::BindOnce(
                                          [](C* ptr, void* ptr_to_free) {
                                            ptr->~C();
                                            free(ptr_to_free);
                                          },
                                          ptr_, ptr_to_free_));
    ptr_ = nullptr;
    ptr_to_free_ = nullptr;
    task_runner_ = nullptr;
  }

  // Return whether we own anything.  Note that this does not guarantee that any
  // previously owned object has been destroyed.  In particular, it will return
  // true immediately after a call to reset(), though the underlying object
  // might still be pending destruction on the other thread.
  bool is_null() const { return !ptr_; }

  // TODO(liberato): Split |ptr_| and |task_runner_| into a weakref-style base.
  // It can provide weak calls and a helper to return weak thread-hopping
  // callbacks.  Really simple solution is to provide a thread-hopping callback
  // to a method with no bound arguments, else the template goop gets ugly.
  // The caller can bind any bound arguments it likes to that callback, rather
  // than specifying them in the original bind.

 protected:
  // The object that we own.  It will be destructed on |task_runner_|.  Do not
  // call anything on this from other threads; it might not have been
  // constructed yet.
  C* ptr_ = nullptr;

  // This is the originally allocated pointer, which we preserve regardless of
  // how we're move-assigned with different types.  This ensures that we can
  // still free the memory.  If we ever support taking ownership of externally
  // constructed objects, then this should be left at nullptr, and we would just
  // delete |ptr_| on the proper thread.  However, if we want to support custom
  // new / delete operators on |C|, we really shouldn't do it this way.
  void* ptr_to_free_ = nullptr;

  // The task runner on which all access to |ptr_| should happen.
  scoped_refptr<base::SequencedTaskRunner> task_runner_;

  // For move construction / assignment.
  template <typename U>
  friend class PostOnlyOwner;

  // To preserve ownership semantics, we disallow copy construction / copy
  // assignment.  Move construction / assignment is fine.
  DISALLOW_COPY_AND_ASSIGN(PostOnlyOwner<C>);
};

}  // namespace media

#endif  // MEDIA_BASE_POST_ONLY_OWNER_H_
