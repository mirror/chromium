// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_BASE_ANDROID_TEST_DESTRUCTION_OBSERVABLE_H_
#define MEDIA_BASE_ANDROID_TEST_DESTRUCTION_OBSERVABLE_H_

#include "base/callback_helpers.h"
#include "base/memory/weak_ptr.h"

namespace media {

class DestructionObserver;

// DestructionObservable is a base class for use in tests that want to make
// assertions about the lifetime of objects that they don't own. It lets you a
// create a DestructionObserver on which expectations can be set without holding
// a reference to the observable.
struct DestructionObservable {
  DestructionObservable();
  virtual ~DestructionObservable();

  std::unique_ptr<DestructionObserver> CreateDestructionObserver();

  base::ScopedClosureRunner destruction_cb;
  DISALLOW_COPY_AND_ASSIGN(DestructionObservable);
};

class DestructionObserver {
 public:
  DestructionObserver(DestructionObservable* observable);
  ~DestructionObserver();

  // Sets an expectation that the observable is destructed before the observer.
  void ExpectDestruction();

  // Sets an expectation that the observable is not destructed before the
  // observer.
  void ExpectNoDestruction();

  // Verifies the last expectation and clears it.
  void VerifyAndClearExpectation();

 private:
  enum class Expectation {
    kNone,
    kDestruction,
    kNoDestruction,
  };

  void OnObservableDestructed();
  void VerifyExpectation();

  // Whether the observable has been destructed.
  bool destructed_;
  Expectation expectation_;

  base::WeakPtrFactory<DestructionObserver> weak_factory_;
  DISALLOW_COPY_AND_ASSIGN(DestructionObserver);
};

}  // namespace media

#endif  // MEDIA_BASE_ANDROID_TEST_DESTRUCTION_OBSERVABLE_H_
