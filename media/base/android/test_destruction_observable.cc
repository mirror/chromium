// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/base/android/test_destruction_observable.h"

#include "base/bind.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace media {

DestructionObservable::DestructionObservable() = default;

DestructionObservable::~DestructionObservable() = default;

std::unique_ptr<DestructionObserver>
DestructionObservable::CreateDestructionObserver() {
  return base::MakeUnique<DestructionObserver>(this);
}

DestructionObserver::DestructionObserver(DestructionObservable* observable)
    : destructed_(false),
      expectation_(Expectation::kNone),
      weak_factory_(this) {
  DCHECK(!observable->destruction_cb.Release());
  observable->destruction_cb.ReplaceClosure(
      base::Bind(&DestructionObserver::OnObservableDestructed,
                 weak_factory_.GetWeakPtr()));
}

DestructionObserver::~DestructionObserver() {
  VerifyExpectation();
}

void DestructionObserver::OnObservableDestructed() {
  destructed_ = true;
  VerifyExpectation();
}

void DestructionObserver::VerifyAndClearExpectation() {
  VerifyExpectation();
  expectation_ = Expectation::kNone;
}

void DestructionObserver::VerifyExpectation() {
  if (expectation_ == Expectation::kDestruction) {
    ASSERT_TRUE(destructed_);
  } else if (expectation_ == Expectation::kNoDestruction) {
    ASSERT_FALSE(destructed_);
  }
}

void DestructionObserver::ExpectDestruction() {
  ASSERT_FALSE(destructed_);
  expectation_ = Expectation::kDestruction;
}

void DestructionObserver::ExpectNoDestruction() {
  ASSERT_FALSE(destructed_);
  expectation_ = Expectation::kNoDestruction;
}

}  // namespace media
