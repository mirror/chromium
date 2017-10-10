// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/u2f/mock_u2f_discovery.h"

#include <utility>

#include "base/threading/thread_task_runner_handle.h"

namespace device {

MockU2fDiscovery::MockObserver::MockObserver() : weak_factory_(this) {}

MockU2fDiscovery::MockObserver::~MockObserver() = default;

void MockU2fDiscovery::MockObserver::OnDeviceKnown(
    std::unique_ptr<U2fDevice> device) {
  OnDeviceKnownStr(device->GetId());
}

void MockU2fDiscovery::MockObserver::OnDeviceAdded(
    std::unique_ptr<U2fDevice> device) {
  OnDeviceAddedStr(device->GetId());
}

void MockU2fDiscovery::MockObserver::OnDeviceRemoved(
    base::StringPiece device_id) {
  OnDeviceRemovedStr(std::string(device_id));
}

base::WeakPtr<MockU2fDiscovery::MockObserver>
MockU2fDiscovery::MockObserver::GetWeakPtr() {
  return weak_factory_.GetWeakPtr();
}

MockU2fDiscovery::MockU2fDiscovery() = default;

MockU2fDiscovery::~MockU2fDiscovery() = default;

void MockU2fDiscovery::KnownDevice(std::unique_ptr<U2fDevice> device) {
  if (observer_)
    observer_->OnDeviceKnown(std::move(device));
}

void MockU2fDiscovery::AddDevice(std::unique_ptr<U2fDevice> device) {
  if (observer_)
    observer_->OnDeviceAdded(std::move(device));
}

void MockU2fDiscovery::RemoveDevice(base::StringPiece device_id) {
  if (observer_)
    observer_->OnDeviceRemoved(device_id);
}

void MockU2fDiscovery::StartSuccess() {
  if (observer_)
    observer_->OnStarted(true);
}

void MockU2fDiscovery::StartFailure() {
  if (observer_)
    observer_->OnStarted(false);
}

// static
void MockU2fDiscovery::StartSuccessAsync() {
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE,
      base::BindOnce(&MockU2fDiscovery::StartSuccess, base::Unretained(this)));
}

// static
void MockU2fDiscovery::StartFailureAsync() {
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE,
      base::BindOnce(&MockU2fDiscovery::StartFailure, base::Unretained(this)));
}

}  // namespace device
