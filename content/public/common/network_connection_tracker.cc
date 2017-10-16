// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/public/common/network_connection_tracker.h"

#include <utility>

#include "base/task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "content/public/common/network_service.mojom.h"

namespace content {

namespace {

// Wraps a |user_callback| when GetConnectionType() is called on a different
// thread than NetworkConnectionTracker's thread.
void OnGetConnectionType(
    scoped_refptr<base::TaskRunner> task_runner,
    const NetworkConnectionTracker::ConnectionTypeCallback& user_callback,
    mojom::ConnectionType connection_type) {
  task_runner->PostTask(
      FROM_HERE,
      base::BindOnce(
          [](NetworkConnectionTracker::ConnectionTypeCallback callback,
             mojom::ConnectionType type) { callback.Run(type); },
          user_callback, connection_type));
}

}  // namespace

NetworkConnectionTracker::NetworkConnectionTracker(
    mojom::NetworkService* network_service)
    : task_runner_(base::ThreadTaskRunnerHandle::Get()),
      initial_connection_type_received_(false),
      connection_type_(mojom::ConnectionType::CONNECTION_UNKNOWN),
      network_change_observer_list_(
          new base::ObserverListThreadSafe<NetworkConnectionObserver>(
              base::ObserverListBase<
                  NetworkConnectionObserver>::NOTIFY_EXISTING_ONLY)),
      binding_(this) {
  // Get NetworkChangeManagerPtr.
  mojom::NetworkChangeManagerPtr manager_ptr;
  mojom::NetworkChangeManagerRequest request(mojo::MakeRequest(&manager_ptr));
  network_service->GetNetworkChangeManager(std::move(request));

  // Request notification from NetworkChangeManagerPtr.
  mojom::NetworkChangeManagerClientPtr client_ptr;
  mojom::NetworkChangeManagerClientRequest client_request(
      mojo::MakeRequest(&client_ptr));
  binding_.Bind(std::move(client_request));
  manager_ptr->RequestNotifications(std::move(client_ptr));
}

NetworkConnectionTracker::~NetworkConnectionTracker() {
  network_change_observer_list_->AssertEmpty();
}

bool NetworkConnectionTracker::GetConnectionType(
    mojom::ConnectionType* type,
    ConnectionTypeCallback callback) {
  base::AutoLock lock(lock_);
  if (!initial_connection_type_received_) {
    if (!task_runner_->RunsTasksInCurrentSequence()) {
      connection_type_callbacks_.push_back(
          base::Bind(&OnGetConnectionType, base::ThreadTaskRunnerHandle::Get(),
                     std::move(callback)));
    } else {
      connection_type_callbacks_.push_back(std::move(callback));
    }
    return false;
  }
  *type = connection_type_;
  return true;
}

// static
bool NetworkConnectionTracker::IsConnectionCellular(
    mojom::ConnectionType type) {
  bool is_cellular = false;
  switch (type) {
    case mojom::ConnectionType::CONNECTION_2G:
    case mojom::ConnectionType::CONNECTION_3G:
    case mojom::ConnectionType::CONNECTION_4G:
      is_cellular = true;
      break;
    case mojom::ConnectionType::CONNECTION_UNKNOWN:
    case mojom::ConnectionType::CONNECTION_ETHERNET:
    case mojom::ConnectionType::CONNECTION_WIFI:
    case mojom::ConnectionType::CONNECTION_NONE:
    case mojom::ConnectionType::CONNECTION_BLUETOOTH:
      is_cellular = false;
      break;
  }
  return is_cellular;
}

void NetworkConnectionTracker::AddNetworkConnectionObserver(
    NetworkConnectionObserver* observer) {
  network_change_observer_list_->AddObserver(observer);
}

void NetworkConnectionTracker::RemoveNetworkConnectionObserver(
    NetworkConnectionObserver* observer) {
  network_change_observer_list_->RemoveObserver(observer);
}

void NetworkConnectionTracker::OnInitialConnectionType(
    mojom::ConnectionType type) {
  base::AutoLock lock(lock_);
  DCHECK(!initial_connection_type_received_);

  initial_connection_type_received_ = true;
  connection_type_ = type;

  while (!connection_type_callbacks_.empty()) {
    connection_type_callbacks_.front().Run(type);
    connection_type_callbacks_.pop_front();
  }
}

void NetworkConnectionTracker::OnNetworkChanged(mojom::ConnectionType type) {
  connection_type_ = type;
  network_change_observer_list_->Notify(
      FROM_HERE, &NetworkConnectionObserver::OnConnectionChanged, type);
}

}  // namespace content
