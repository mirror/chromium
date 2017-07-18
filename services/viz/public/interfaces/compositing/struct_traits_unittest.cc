// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <utility>

#include "components/viz/common/resources/resource_settings.h"
#include "mojo/public/cpp/bindings/binding_set.h"
#include "services/viz/public/interfaces/compositing/traits_test_service.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace viz {

namespace {

class StructTraitsTest : public testing::Test, public mojom::TraitsTestService {
 public:
  StructTraitsTest() = default;

 protected:
  mojom::TraitsTestServicePtr GetTraitsTestProxy() {
    mojom::TraitsTestServicePtr proxy;
    traits_test_bindings_.AddBinding(this, mojo::MakeRequest(&proxy));
    return proxy;
  }

 private:
  // TraitsTestService implementation.
  // void EchoResourceSettings(const ResourceSettings& r,
  //   EchoResourceSettingsCallback callback) override {
  //   std::move(callback).Run(r);
  // }
  void Foo() override {}

  mojo::BindingSet<TraitsTestService> traits_test_bindings_;
  DISALLOW_COPY_AND_ASSIGN(StructTraitsTest);
};

TEST_F(StructTraitsTest, ResourceSettings) {
  // TODO(staraz): Initialize input.
  ResourceSettings input;
  mojom::TraitsTestServicePtr proxy = GetTraitsTestProxy();
  ResourceSettings output;
  // proxy->EchoResourceSettings(input, &output);
  // TODO(staraz): Verify |output|.
}

}  // namespace

}  // namespace viz
