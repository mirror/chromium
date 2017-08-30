// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file contains utility functions and classes that help the
// implementation, and management of the Callback objects.

#ifndef BASE_CALLBACK_INTERNAL_H_
#define BASE_CALLBACK_INTERNAL_H_

#include "base/base_export.h"
#include "base/callback_forward.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"

namespace base {

struct FakeBindState;

namespace internal {

class BindStateBase;
class BindStateBaseRefCounted;

class CallbackBase;
class CallbackBaseCopyable;

template <typename Base, typename Functor, typename... BoundArgs>
struct BindState;

struct BindStateBaseReleaser {
  void operator()(BindStateBase* bind_state) const;
};
using BindStateBasePtr = std::unique_ptr<BindStateBase, BindStateBaseReleaser>;

struct BindStateBaseRefCountTraits {
  static void Destruct(const BindStateBaseRefCounted* bind_state);
};

// BindStateBase is used to provide an opaque handle that the Callback classes
// can use to represent a function object with bound arguments.  It behaves as
// an existential type that is used by a corresponding invoker function to
// perform the function execution.  This allows us to shield the Callback class
// from the types of the bound argument via "type erasure."
class BASE_EXPORT BindStateBase {
 public:
  using InvokeFuncStorage = void(*)();

 protected:
  BindStateBase(InvokeFuncStorage polymorphic_invoke,
                void (*releaser)(const BindStateBase*));
  BindStateBase(InvokeFuncStorage polymorphic_invoke,
                void (*releaser)(const BindStateBase*),
                bool (*is_cancelled)(const BindStateBase*));

  ~BindStateBase() = default;

  friend class CallbackBase;
  friend struct BindStateBaseReleaser;

  bool IsCancelled() const {
    return is_cancelled_(this);
  }

  // In C++, it is safe to cast function pointers to function pointers of
  // another type. It is not okay to use void*. We create a InvokeFuncStorage
  // that that can store our function pointer, and then cast it back to
  // the original type on usage.
  InvokeFuncStorage polymorphic_invoke_;

  // Pointer to a function that will properly destroy |this|. BindStateBase
  // doesn't use virtual methods including virtual desructor, as vtables for
  // every BindState template instantiation results in a lot of bloat.
  void (*releaser_)(const BindStateBase*);
  bool (*is_cancelled_)(const BindStateBase*);

  DISALLOW_COPY_AND_ASSIGN(BindStateBase);
};

// The ref-counted extension of BindStateBase for copyable Callback classes.
class BASE_EXPORT BindStateBaseRefCounted
    : public BindStateBase,
      public RefCountedThreadSafe<BindStateBaseRefCounted,
                                  BindStateBaseRefCountTraits> {
 public:
  REQUIRE_ADOPTION_FOR_REFCOUNTED_TYPE();

  static BindStateBasePtr AsBindStateBase(
      scoped_refptr<BindStateBaseRefCounted>);

 private:
  BindStateBaseRefCounted(InvokeFuncStorage polymorphic_invoke,
                          void (*destructor)(const BindStateBase*));
  BindStateBaseRefCounted(InvokeFuncStorage polymorphic_invoke,
                          void (*destructor)(const BindStateBase*),
                          bool (*is_cancelled)(const BindStateBase*));

  ~BindStateBaseRefCounted() = default;

  friend struct BindStateBaseRefCountTraits;
  friend class RefCountedThreadSafe<BindStateBaseRefCounted,
                                    BindStateBaseRefCountTraits>;
  friend class CallbackBaseCopyable;

  // Whitelist subclasses that access the destructor of BindStateBase.
  template <typename Base, typename Functor, typename... BoundArgs>
  friend struct BindState;
  friend struct ::base::FakeBindState;

  // Pointer to a function that will properly destroy |this|.
  void (*destructor_)(const BindStateBase*);

  DISALLOW_COPY_AND_ASSIGN(BindStateBaseRefCounted);
};

// Holds the Callback methods that don't require specialization to reduce
// template bloat.
// CallbackBase is a direct base class of OnceCallback, and CallbackBaseCopyable
// uses CallbackBase for its implementation.
class BASE_EXPORT CallbackBase {
 public:
  // CallbackBase is movable.
  CallbackBase(CallbackBase&& other);
  CallbackBase& operator=(CallbackBase&& other);

  // CallbackBase is not copyable.
  CallbackBase(const CallbackBase& other) = delete;
  CallbackBase& operator=(const CallbackBase& other) = delete;

  // Conversion from CallbackBaseCopyable.
  explicit CallbackBase(const CallbackBaseCopyable& other);
  explicit CallbackBase(CallbackBaseCopyable&& other);
  CallbackBase& operator=(const CallbackBaseCopyable& other);
  CallbackBase& operator=(CallbackBaseCopyable&& other);

  // Returns true if Callback is null (doesn't refer to anything).
  bool is_null() const { return !bind_state_; }
  explicit operator bool() const { return !is_null(); }

  // Returns true if the callback invocation will be nop due to an cancellation.
  // It's invalid to call this on uninitialized callback.
  bool IsCancelled() const;

  // Returns the Callback into an uninitialized state.
  void Reset();

 protected:
  using InvokeFuncStorage = BindStateBase::InvokeFuncStorage;

  // Returns true if this callback equals |other|. |other| may be null.
  bool EqualsInternal(const CallbackBase& other) const;

  // Allow initializing of |bind_state_| via the constructor to avoid default
  // initialization of the scoped_refptr.
  explicit CallbackBase(BindStateBase* bind_state);

  InvokeFuncStorage polymorphic_invoke() const {
    return bind_state_->polymorphic_invoke_;
  }

  // Force the destructor to be instantiated inside this translation unit so
  // that our subclasses will not get inlined versions.  Avoids more template
  // bloat.
  ~CallbackBase();

  BindStateBasePtr bind_state_;
};

// CallbackBase<Copyable> is a direct base class of Copyable Callbacks.
class BASE_EXPORT CallbackBaseCopyable {
 public:
  // CallbackBaseCopyable is moveable.
  CallbackBaseCopyable(CallbackBaseCopyable&& c);
  CallbackBaseCopyable& operator=(CallbackBaseCopyable&& c);

  // CallbackBaseCopyable is copyable.
  CallbackBaseCopyable(const CallbackBaseCopyable& c);
  CallbackBaseCopyable& operator=(const CallbackBaseCopyable& c);

  // Returns true if Callback is null (doesn't refer to anything).
  bool is_null() const { return !bind_state_; }
  explicit operator bool() const { return !is_null(); }

  // Returns true if the callback invocation will be nop due to an cancellation.
  // It's invalid to call this on uninitialized callback.
  bool IsCancelled() const;

  // Returns the Callback into an uninitialized state.
  void Reset();

 protected:
  using InvokeFuncStorage = BindStateBase::InvokeFuncStorage;
  friend class CallbackBase;

  // Returns true if this callback equals |other|. |other| may be null.
  bool EqualsInternal(const CallbackBaseCopyable& other) const;

  // Allow initializing of |bind_state_| via the constructor to avoid default
  // initialization of the scoped_refptr.
  explicit CallbackBaseCopyable(BindStateBaseRefCounted* bind_state);

  InvokeFuncStorage polymorphic_invoke() const {
    return bind_state_->polymorphic_invoke_;
  }

  // Force the destructor to be instantiated inside this translation unit so
  // that our subclasses will not get inlined versions.  Avoids more template
  // bloat.
  ~CallbackBaseCopyable();

  scoped_refptr<BindStateBaseRefCounted> bind_state_;
};

}  // namespace internal
}  // namespace base

#endif  // BASE_CALLBACK_INTERNAL_H_
