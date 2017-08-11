// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/bind.h"
#include "base/callback.h"
#include "base/macros.h"
#include "base/run_loop.h"
#include "base/test/scoped_task_environment.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "services/service_manager/public/cpp/binder_registry.h"
#include "services/service_manager/public/cpp/connector.h"
#include "services/service_manager/public/cpp/service.h"
#include "services/service_manager/public/cpp/test/test_connector_factory.h"
#include "services/service_manager/tests/test.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace service_manager {

namespace {

class TestServiceImpl : public Service {
 public:
  TestServiceImpl() = default;
  ~TestServiceImpl() override = default;

  BinderRegistryWithArgs<const BindSourceInfo&>& registry() {
    return registry_;
  }

  // Service:
  void OnBindInterface(const BindSourceInfo& source_info,
                       const std::string& interface_name,
                       mojo::ScopedMessagePipeHandle interface_pipe) override {
    registry_.BindInterface(interface_name, std::move(interface_pipe),
                            source_info);
  }

 private:
  BinderRegistryWithArgs<const BindSourceInfo&> registry_;

  DISALLOW_COPY_AND_ASSIGN(TestServiceImpl);
};

class TestCImpl : public TestC {
 public:
  TestCImpl() : binding_(this) {}
  ~TestCImpl() override = default;

  void BindRequest(TestCRequest request) { binding_.Bind(std::move(request)); }

  // TestC:
  void C(CCallback callback) override { std::move(callback).Run(); }

 private:
  mojo::Binding<TestC> binding_;

  DISALLOW_COPY_AND_ASSIGN(TestCImpl);
};

}  // namespace

TEST(ServiceManagerTestSupport, TestConnectorFactory) {
  base::test::ScopedTaskEnvironment task_environment;

  static const std::string kTestSourceName = "worst service ever";
  TestCImpl c_impl;
  TestServiceImpl service;
  service.registry().AddInterface(base::Bind(
      [](TestCImpl* c_impl, TestCRequest request, const BindSourceInfo& info) {
        EXPECT_EQ(kTestSourceName, info.identity.name());
        c_impl->BindRequest(std::move(request));
      },
      &c_impl));

  TestConnectorFactory factory(&service);
  factory.set_source_identity(Identity(kTestSourceName));
  std::unique_ptr<Connector> connector = factory.CreateConnector();

  TestCPtr c;
  connector->BindInterface("ignored", &c);

  base::RunLoop loop;
  c->C(loop.QuitClosure());
  loop.Run();
}

}  // namespace service_manager
