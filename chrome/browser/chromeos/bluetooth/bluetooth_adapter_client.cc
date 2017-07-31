// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/bluetooth/bluetooth_adapter_client.h"

#include "ash/public/interfaces/constants.mojom.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/common/pref_names.h"
#include "components/prefs/pref_service.h"
#include "components/user_manager/user_manager.h"
#include "content/public/common/service_manager_connection.h"
#include "services/service_manager/public/cpp/connector.h"

BluetoothAdapterClient::BluetoothAdapterClient() : binding_(this) {
  service_manager::Connector* connector =
      content::ServiceManagerConnection::GetForProcess()->GetConnector();
  connector->BindInterface(ash::mojom::kServiceName,
                           &bluetooth_adapter_controller_);
  ash::mojom::BluetoothAdapterClientPtr client;
  binding_.Bind(mojo::MakeRequest(&client));
  bluetooth_adapter_controller_->SetClient(std::move(client));
}

BluetoothAdapterClient::~BluetoothAdapterClient() {}

void BluetoothAdapterClient::OnAdapterPowerSet(bool enabled) {
  PrefService* prefs = nullptr;
  if (user_manager::UserManager::Get()->IsUserLoggedIn()) {
    prefs = ProfileManager::GetActiveUserProfile()->GetPrefs();
    prefs->SetBoolean(prefs::kBluetoothAdapterEnabled, enabled);
  } else {
    prefs = g_browser_process->local_state();
    prefs->SetBoolean(prefs::kSystemBluetoothAdapterEnabled, enabled);
  }
  LOG(ERROR) << "bluetooth power set " << enabled;
}
