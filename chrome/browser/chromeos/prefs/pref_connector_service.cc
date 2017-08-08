// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/prefs/pref_connector_service.h"

#include "chrome/browser/chromeos/profiles/profile_helper.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "components/user_manager/user_manager.h"
#include "content/public/browser/browser_context.h"
#include "mojo/public/cpp/bindings/interface_request.h"
#include "services/service_manager/public/cpp/connector.h"
#include "services/service_manager/public/cpp/service_context.h"

AshPrefConnector::AshPrefConnector() {
  registry_.AddInterface<ash::mojom::PrefConnector>(base::Bind(
      &AshPrefConnector::BindConnectorRequest, base::Unretained(this)));
}

AshPrefConnector::~AshPrefConnector() = default;

void AshPrefConnector::GetPrefStoreConnectorForUser(
    const AccountId& account_id,
    prefs::mojom::PrefStoreConnectorRequest request) {
  auto* user = user_manager::UserManager::Get()->FindUser(account_id);
  if (!user || !user->is_profile_created())
    return;

  auto* profile = chromeos::ProfileHelper::Get()->GetProfileByUserUnsafe(user);
  content::BrowserContext::GetConnectorFor(profile)->BindInterface(
      prefs::mojom::kServiceName, std::move(request));
}

void AshPrefConnector::BindConnectorRequest(
    ash::mojom::PrefConnectorRequest request) {
  connector_bindings_.AddBinding(this, std::move(request));
}

void AshPrefConnector::OnStart() {}

void AshPrefConnector::OnBindInterface(
    const service_manager::BindSourceInfo& source_info,
    const std::string& interface_name,
    mojo::ScopedMessagePipeHandle interface_pipe) {
  // N.B. This check is important as not doing it would allow one user to read
  // another user's prefs.
  // TODO(beng): This should be obsoleted by Service Manager user id routing.
  if (context()->identity().user_id() != source_info.identity.user_id()) {
    LOG(WARNING) << "Blocked service instance="
                 << source_info.identity.instance()
                 << ", name=" << source_info.identity.name()
                 << ", user_id=" << source_info.identity.user_id()
                 << " from connecting to the active profile's pref service.";
    return;
  }
  registry_.BindInterface(interface_name, std::move(interface_pipe));
}
