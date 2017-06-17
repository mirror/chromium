// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/arc/obb_mounter/arc_obb_mounter_bridge.h"

#include <future>

#include "base/bind.h"
#include "base/logging.h"
#include "chromeos/dbus/arc_obb_mounter_client.h"
#include "chromeos/dbus/dbus_thread_manager.h"
#include "chromeos/disks/disk_mount_manager.h"
#include "components/arc/arc_bridge_service.h"

using chromeos::disks::DiskMountManager;

namespace arc {

namespace {

// Used to convert mojo Callback to VoidDBusMethodCallback.
void RunObbCallback(const base::Callback<void(bool)>& callback,
                    chromeos::DBusMethodCallStatus result) {
  callback.Run(result == chromeos::DBUS_METHOD_CALL_SUCCESS);
}

}  // namespace

ArcObbMounterBridge::ArcObbMounterBridge(ArcBridgeService* bridge_service)
    : ArcService(bridge_service), binding_(this) {
  arc_bridge_service()->obb_mounter()->AddObserver(this);
  DCHECK(DiskMountManager::GetInstance());
  DiskMountManager::GetInstance()->AddObserver(this);
}

ArcObbMounterBridge::~ArcObbMounterBridge() {
  arc_bridge_service()->obb_mounter()->RemoveObserver(this);
  DiskMountManager::GetInstance()->RemoveObserver(this);
}

void ArcObbMounterBridge::OnInstanceReady() {
  mojom::ObbMounterInstance* obb_mounter_instance =
      ARC_GET_INSTANCE_FOR_METHOD(arc_bridge_service()->obb_mounter(), Init);
  DCHECK(obb_mounter_instance);
  mojom::ObbMounterHostPtr host_proxy;
  binding_.Bind(mojo::MakeRequest(&host_proxy));
  obb_mounter_instance->Init(std::move(host_proxy));
  remount_all_ =
      std::async(std::launch::async, &ArcObbMounterBridge::RemountAll, this);
}

void ArcObbMounterBridge::MountObb(const std::string& obb_file,
                                   const std::string& target_path,
                                   int32_t owner_gid,
                                   const MountObbCallback& callback) {
  chromeos::DBusThreadManager::Get()->GetArcObbMounterClient()->MountObb(
      obb_file, target_path, owner_gid, base::Bind(&RunObbCallback, callback));
}

void ArcObbMounterBridge::UnmountObb(const std::string& target_path,
                                     const UnmountObbCallback& callback) {
  chromeos::DBusThreadManager::Get()->GetArcObbMounterClient()->UnmountObb(
      target_path, base::Bind(&RunObbCallback, callback));
}

void ArcObbMounterBridge::OnDiskEvent(
    chromeos::disks::DiskMountManager::DiskEvent event,
    const chromeos::disks::DiskMountManager::Disk* disk) {}

void ArcObbMounterBridge::OnDeviceEvent(
    chromeos::disks::DiskMountManager::DeviceEvent event,
    const std::string& device_path) {}

void ArcObbMounterBridge::OnMountEvent(
    DiskMountManager::MountEvent event,
    chromeos::MountError error_code,
    const chromeos::disks::DiskMountManager::MountPointInfo& mount_info) {
  if (error_code != chromeos::MountError::MOUNT_ERROR_NONE) {
    return;
  }
  const chromeos::disks::DiskMountManager::Disk* disk =
      DiskMountManager::GetInstance()->FindDiskBySourcePath(
          mount_info.source_path);
  std::string fsUuid, deviceLabel;
  mojom::DeviceType deviceType;
  if (disk) {
    fsUuid = disk->fs_uuid();
    deviceLabel = disk->device_label();
    deviceType = static_cast<mojom::DeviceType>(disk->device_type());
  }
  mojom::VolumeMounterInstance* volume_mounter_instance =
      ARC_GET_INSTANCE_FOR_METHOD(arc_bridge_service()->volume_mounter(),
                                  OnMountEvent);
  if (!volume_mounter_instance) {
    return;
  }
  volume_mounter_instance->OnMountEvent(mojom::MountPointInfo::New(
      static_cast<mojom::MountEvent>(event), mount_info.source_path,
      mount_info.mount_path, fsUuid, deviceLabel, deviceType));
}

void ArcObbMounterBridge::OnFormatEvent(
    chromeos::disks::DiskMountManager::FormatEvent event,
    chromeos::FormatError error_code,
    const std::string& device_path) {}

void ArcObbMounterBridge::RemountAll() {
  for (const auto& keyValue : DiskMountManager::GetInstance()->mount_points()) {
    OnMountEvent(DiskMountManager::MountEvent::MOUNTING,
                 chromeos::MountError::MOUNT_ERROR_NONE, keyValue.second);
  }
}

}  // namespace arc
