// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/test/scoped_task_environment.h"
#include "mojo/public/cpp/bindings/binding_set.h"
#include "sandbox/mac/mojo/seatbelt_extension_token_struct_traits.h"
#include "sandbox/mac/mojo/traits_test_service.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace sandbox {
namespace {

class StructTraitsTest : public testing::Test,
                         sandbox::mac::mojom::TraitsTestService {
 public:
  StructTraitsTest() {}

  sandbox::mac::mojom::TraitsTestServicePtr GetProxy() {
    sandbox::mac::mojom::TraitsTestServicePtr proxy;
    bindings_.AddBinding(this, mojo::MakeRequest(&proxy));
    return proxy;
  }

 private:
  // TraitsTestService:
  void EchoSeatbeltExtensionToken(
      const sandbox::SeatbeltExtensionToken& token,
      EchoSeatbeltExtensionTokenCallback callback) override {
    std::move(callback).Run(token);
  }

  base::test::ScopedTaskEnvironment task_environment_;
  mojo::BindingSet<sandbox::mac::mojom::TraitsTestService> bindings_;
};

TEST_F(StructTraitsTest, SeatbeltExtensionToken) {
  auto token = sandbox::SeatbeltExtensionToken::NewForTesting("hello world");
  sandbox::SeatbeltExtensionToken output;

  GetProxy()->EchoSeatbeltExtensionToken(*token.get(), &output);
  EXPECT_EQ("hello world", output.token());
}

}  // namespace
}  // namespace sandbox
