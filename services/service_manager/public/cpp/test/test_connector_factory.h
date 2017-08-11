// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_SERVICE_MANAGER_PUBLIC_CPP_TEST_TEST_CONNECTOR_FACTORY_H_
#define SERVICES_SERVICE_MANAGER_PUBLIC_CPP_TEST_TEST_CONNECTOR_FACTORY_H_

#include <memory>

#include "base/macros.h"
#include "services/service_manager/public/cpp/connector.h"
#include "services/service_manager/public/interfaces/connector.mojom.h"

namespace service_manager {

class Service;

// Creates Connector instances which route BindInterface requests directly to
// a given Service implementation. Useful for testing production code which is
// parameterized over a Connector, while bypassing all the Service Manager
// machinery.
class TestConnectorFactory {
 public:
  // Constructs a new TestConnectorFactory which creates Connectors whose
  // requests are routed directly to |service|. |service| is not owned and must
  // outlive this TestConnectorFactory instance.
  explicit TestConnectorFactory(Service* service);
  ~TestConnectorFactory();

  void set_source_identity(const Identity& identity) {
    source_identity_ = identity;
  }

  const Identity& source_identity() const { return source_identity_; }

  std::unique_ptr<Connector> CreateConnector();

 private:
  Identity source_identity_;

  std::unique_ptr<mojom::Connector> impl_;

  DISALLOW_COPY_AND_ASSIGN(TestConnectorFactory);
};

}  // namespace service_manager

#endif  // SERVICES_SERVICE_MANAGER_PUBLIC_CPP_TEST_TEST_CONNECTOR_FACTORY_H_
