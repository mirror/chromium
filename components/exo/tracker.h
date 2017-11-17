// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_EXO_TRACKER_H_
#define COMPONENTS_EXO_TRACKER_H_

namespace exo {

template <typename T, typename O>
using RegistrationFunc = void (T::*)(O*);

// Pointer with observing target event.
// This reference is useful when a member variable of pointer is not const and
// needs to register/unregister observer everytime the pointer value is updated.
template <typename T,
          typename O,
          RegistrationFunc<T, O> ADD = &T::AddObserver,
          RegistrationFunc<T, O> REMOVE = &T::RemoveObserver>
class Tracker {
 public:
  Tracker() : Tracker(nullptr, nullptr) {}
  Tracker(T* target, O* observer) : target_(target), observer_(observer) {
    Invoke(ADD);
  }
  ~Tracker() { reset(); }

  T* get() { return target_; }
  T* operator->() { return target_; }
  T& operator*() { return *target_; }
  operator bool() { return target_ != nullptr; }

  void reset(T* target, O* observer) {
    Invoke(REMOVE);
    target_ = target;
    observer_ = observer;
    Invoke(ADD);
  }

  void reset() { reset(nullptr, nullptr); }

 private:
  void Invoke(RegistrationFunc<T, O> func) {
    DCHECK(!!target_ == !!observer_);
    if (!target_ || !observer_)
      return;
    (target_->*func)(observer_);
  }
  T* target_;
  O* observer_;

  DISALLOW_COPY_AND_ASSIGN(Tracker);
};

}  // namespace exo

#endif  // COMPONENTS_EXO_TRACKER_H_
