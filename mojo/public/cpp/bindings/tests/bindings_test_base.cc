// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/public/cpp/bindings/tests/bindings_test_base.h"

#include "mojo/public/cpp/bindings/connector.h"

namespace mojo {

BindingsTestBase::BindingsTestBase() {
  SetupSerializationBehavior(GetParam());
}

BindingsTestBase::~BindingsTestBase() = default;

// static
void BindingsTestBase::SetupSerializationBehavior(
    BindingsTestSerializationMode mode) {
  switch (mode) {
    case BindingsTestSerializationMode::kSerializeBeforeSend:
      Connector::OverrideDefaultSerializationBehavior(
          Connector::OutgoingSerializationMode::kEager,
          Connector::IncomingSerializationMode::kDispatchAsIs);
      break;
    case BindingsTestSerializationMode::kSerializeBeforeDispatch:
      Connector::OverrideDefaultSerializationBehavior(
          Connector::OutgoingSerializationMode::kLazy,
          Connector::IncomingSerializationMode::kSerializeBeforeDispatch);
      break;
    case BindingsTestSerializationMode::kNeverSerialize:
      Connector::OverrideDefaultSerializationBehavior(
          Connector::OutgoingSerializationMode::kLazy,
          Connector::IncomingSerializationMode::kDispatchAsIs);
      break;
  }
}

}  // namespace mojo
