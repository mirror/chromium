// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/service_manager/ServiceManager.h"

#include <memory>

#include "base/macros.h"
#include "core/mojo/MojoHandle.h"
#include "mojo/public/cpp/system/core.h"
#include "platform/CrossThreadFunctional.h"
#include "platform/WebTaskRunner.h"
#include "platform/wtf/StdLibExtras.h"
#include "platform/wtf/ThreadSpecific.h"
#include "public/platform/Platform.h"
#include "services/service_manager/public/cpp/connector.h"
#include "services/service_manager/public/cpp/identity.h"

namespace blink {

template <>
struct CrossThreadCopier<service_manager::mojom::ConnectorRequest> {
  STATIC_ONLY(CrossThreadCopier);
  using Type = service_manager::mojom::ConnectorRequest;
  static service_manager::mojom::ConnectorRequest Copy(
      service_manager::mojom::ConnectorRequest request) {
    return request;  // This is in fact a move.
  }
};

namespace {

class ConnectorHolder {
 public:
  ConnectorHolder() {
    service_manager::mojom::ConnectorRequest request;
    connector_ = service_manager::Connector::Create(&request);
    Platform::Current()->MainThread()->GetWebTaskRunner()->PostTask(
        BLINK_FROM_HERE,
        CrossThreadBind(&ConnectorHolder::BindRequestOnMainThread,
                        WTF::Passed(std::move(request))));
  }

  service_manager::Connector* connector() const { return connector_.get(); }

 private:
  static void BindRequestOnMainThread(
      service_manager::mojom::ConnectorRequest request) {
    Platform::Current()->GetConnector()->BindConnectorRequest(
        std::move(request));
  }

  std::unique_ptr<service_manager::Connector> connector_;

  DISALLOW_COPY_AND_ASSIGN(ConnectorHolder);
};

}  // namespace

// static
void ServiceManager::bindInterface(const String& service_name,
                                   const String& interface_name,
                                   MojoHandle* request_handle) {
  // We lazily clone a per-thread Connector client so that each thread only has
  // to hop to the main thread for the first bindInterface() call it makes.
  DEFINE_THREAD_SAFE_STATIC_LOCAL(ThreadSpecific<ConnectorHolder>,
                                  connector_holder,
                                  new ThreadSpecific<ConnectorHolder>);
  connector_holder->connector()->BindInterface(
      service_manager::Identity(std::string(service_name.Utf8().data())),
      std::string(interface_name.Utf8().data()),
      mojo::ScopedMessagePipeHandle::From(request_handle->TakeHandle()));
}

}  // namespace blink
