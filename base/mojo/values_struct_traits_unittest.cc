// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <utility>

#include "base/message_loop/message_loop.h"
#include "base/mojo/values_test.mojom.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace base {

class ValuesTest : public mojom::ValuesTest {
 public:
  explicit ValuesTest(mojo::InterfaceRequest<mojom::ValuesTest> request)
      : binding_(this, std::move(request)) {}

  void BounceValue(base::Value in, BounceValueCallback callback) override {
    std::move(callback).Run(in);
  }

 private:
  mojo::Binding<mojom::ValuesTest> binding_;
};

class ValuesStructTraitsTest : public ::testing::Test {
 public:
  ValuesStructTraitsTest() : tester_(MakeRequest(&proxy_)) {}

 protected:
  mojom::ValuesTestPtr proxy_;
  ValuesTest tester_;

 private:
  base::MessageLoop message_loop;
};

}  // namespace base
