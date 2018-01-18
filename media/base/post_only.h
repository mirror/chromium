// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_BASE_POST_ONLY_TASK_H_
#define MEDIA_BASE_POST_ONLY_TASK_H_

#include "base/bind.h"
#include "base/callback.h"
#include "base/location.h"
#include "base/memory/ptr_util.h"
#include "base/sequenced_task_runner.h"
#include "media/base/bind_to_current_loop.h"
#include "media/base/media_export.h"

namespace media {

// Owner for |C|, which should run on some fixed sequence.  This class will
// handle construction, method calls (posts), and destruction on that sequence.
template <typename C>
class PostOnlyOwner {
 public:
  // Allow empty owners.
  PostOnlyOwner() {}

  // Post construction of |C| with |args| to |task_runner|.  One may post to
  // it immediately upon return.
  template <typename... Args>
  PostOnlyOwner(scoped_refptr<base::SequencedTaskRunner> task_runner,
                Args... args)
      : ptr_((C*)malloc(sizeof(ConstructionHelper))),
        task_runner_(std::move(task_runner)) {
    // Post construction to |task_runner_|.
    task_runner_->PostTask(
        FROM_HERE,
        base::BindOnce(
            [](ConstructionHelper* c, Args... args) {
              new (c) ConstructionHelper(std::move(args)...);
            },
            base::Unretained(static_cast<ConstructionHelper*>(ptr_)), args...));
  }

  ~PostOnlyOwner() { reset(); }

  // Move construction is supported from any type that's compatible with |C|.
  template <typename U>
  PostOnlyOwner(PostOnlyOwner<U>&& other)
      : ptr_(other.ptr_), task_runner_(std::move(other.task_runner_)) {
    other.ptr_ = nullptr;
  }

  // Move assignment is supported for any type that's compatible with |C|.
  template <typename U>
  PostOnlyOwner<C>& operator=(PostOnlyOwner<U>&& other) {
    if (other.is_null()) {
      reset();
    } else {
      ptr_ = other.ptr_;
      other.ptr_ = nullptr;
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

    task_runner_->DeleteSoon(FROM_HERE, ptr_);
    ptr_ = nullptr;
    task_runner_ = nullptr;
  }

  // Return whether we own anything.  Note that this does not guarantee that any
  // previously owned object has been destroyed.  In particular, it will return
  // true immediately after a call to reset(), though the underlying object
  // might still be pending destruction on the other thread.
  bool is_null() const { return !ptr_; }

  // TODO(liberato): Add a helper to return thread-hopping callbacks.  These
  // will have to deal with the case where |ptr_| is destroyed before they run.

 protected:
  // The object that we own.  It will be destructed on |task_runner_|.  Do not
  // call anything on this from other threads; it might not have been
  // constructed yet.
  C* ptr_ = nullptr;

  // The task runner on which all access to |ptr_| should happen.
  scoped_refptr<base::SequencedTaskRunner> task_runner_;

  // Helper class to make a protected constructor available.  This makes it easy
  // to hide constructors, to prevent calling new directly accidentally.
  class ConstructionHelper : public C {
   public:
    template <typename... Args>
    ConstructionHelper(Args... args) : C(std::move(args)...) {}
  };

  // For move construction / assignment.
  template <typename U>
  friend class PostOnlyOwner;

  // To preserve ownership semantics, we disallow copy construction / copy
  // assignment.  Move construction / assignment is fine.
  DISALLOW_COPY_AND_ASSIGN(PostOnlyOwner<C>);
};

}  // namespace media

#endif  // MEDIA_BASE_POST_ONLY_TASK_H_
