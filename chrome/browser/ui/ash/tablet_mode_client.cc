// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/ash/tablet_mode_client.h"

#include "ash/public/interfaces/constants.mojom.h"
#include "chrome/browser/ui/ash/tablet_mode_client_observer.h"
#include "content/public/common/service_manager_connection.h"
#include "content/public/common/service_names.mojom.h"
#include "services/service_manager/public/cpp/connector.h"

namespace {

TabletModeClient* g_instance = nullptr;

}  // namespace

TabletModeClient::TabletModeClient() : binding_(this) {
  DCHECK(!g_instance);
  g_instance = this;
}

TabletModeClient::~TabletModeClient() {
  DCHECK_EQ(this, g_instance);
  g_instance = nullptr;
}

void TabletModeClient::Init() {
  // Bind this object as the client.
  ash::mojom::TabletModeClientPtr client;
  binding_.Bind(mojo::MakeRequest(&client));

  ConnectToTabletModeController();
  tablet_mode_controller_->SetClient(std::move(client));
}

// static
TabletModeClient* TabletModeClient::Get() {
  return g_instance;
}

void TabletModeClient::AddObserver(TabletModeClientObserver* observer) {
  observers_.AddObserver(observer);
}

void TabletModeClient::RemoveObserver(TabletModeClientObserver* observer) {
  observers_.RemoveObserver(observer);
}

void TabletModeClient::OnTabletModeToggled(bool enabled) {
  LOG(ERROR) << "JAMES OnTabletModeToggled " << enabled;
  tablet_mode_enabled_ = enabled;
  for (auto& observer : observers_)
    observer.OnTabletModeToggled(enabled);
}

void TabletModeClient::FlushForTesting() {
  tablet_mode_controller_.FlushForTesting();
}

//JAMES inline this if tests don't need it
void TabletModeClient::ConnectToTabletModeController() {
  // Tests may bind to their own TabletModeController.
  if (tablet_mode_controller_)
    return;

  content::ServiceManagerConnection::GetForProcess()
      ->GetConnector()
      ->BindInterface(ash::mojom::kServiceName, &tablet_mode_controller_);
}
