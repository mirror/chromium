// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_SERVICES_ASSISTANT_SERVICE_H_
#define CHROMEOS_SERVICES_ASSISTANT_SERVICE_H_

#include <memory>
#include <string>

#include "base/memory/weak_ptr.h"
#include "base/optional.h"
#include "base/time/time.h"
#include "chromeos/services/assistant/public/interfaces/assistant.mojom.h"
#include "mojo/public/cpp/bindings/binding_set.h"
#include "services/identity/public/interfaces/identity_manager.mojom.h"
#include "services/service_manager/public/cpp/binder_registry.h"
#include "services/service_manager/public/cpp/service.h"

class GoogleServiceAuthError;

namespace chromeos {
namespace assistant {

class AssistantManagerService;

class Service : public mojom::AssistantConnector,
                public service_manager::Service {
 public:
  class AssistantServiceDelegate {
   public:
    virtual ~AssistantServiceDelegate() = default;
    virtual void RequestAccessToken(
        identity::mojom::IdentityManager::GetAccessTokenCallback callback) {}
  };

  explicit Service(AssistantServiceDelegate* delegate);
  ~Service() override;

 private:
  // service_manager::Service overrides
  void OnStart() override;
  void OnBindInterface(const service_manager::BindSourceInfo& source_info,
                       const std::string& interface_name,
                       mojo::ScopedMessagePipeHandle interface_pipe) override;

  // mojom::AssistantConnector overrides.
  void OnActiveUserChanged(
      const base::Optional<std::string>& account_id) override;

  void BindConnectorRequest(mojom::AssistantConnectorRequest request);

  void RefreshToken();
  void GetAccessTokenCallback(const base::Optional<std::string>& token,
                              base::Time expiration_time,
                              const GoogleServiceAuthError& error);

  AssistantServiceDelegate* delegate_;
  mojo::Binding<mojom::AssistantConnector> connector_binding_;

  service_manager::BinderRegistry registry_;
  base::Optional<std::string> account_id_;

  std::unique_ptr<AssistantManagerService> assistant_manager_service_;

  base::WeakPtrFactory<Service> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(Service);
};

}  // namespace assistant
}  // namespace chromeos

#endif  // CHROMEOS_SERVICES_ASSISTANT_SERVICE_H_
