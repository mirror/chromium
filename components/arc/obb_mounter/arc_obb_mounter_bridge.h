// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_ARC_OBB_MOUNTER_ARC_OBB_MOUNTER_BRIDGE_H_
#define COMPONENTS_ARC_OBB_MOUNTER_ARC_OBB_MOUNTER_BRIDGE_H_

#include <future>
#include <string>

#include "base/macros.h"
#include "chromeos/disks/disk_mount_manager.h"
#include "components/arc/arc_service.h"
#include "components/arc/common/obb_mounter.mojom.h"
#include "components/arc/common/volume_mounter.mojom.h"
#include "components/arc/instance_holder.h"
#include "mojo/public/cpp/bindings/binding.h"

namespace arc {

class ArcBridgeService;

// TODO: Change the name to MounterBridge.
// This class handles OBB mount/unmount requests from Android.
class ArcObbMounterBridge
    : public ArcService,
      public chromeos::disks::DiskMountManager::Observer,
      public InstanceHolder<mojom::ObbMounterInstance>::Observer,
      public mojom::ObbMounterHost {
 public:
  explicit ArcObbMounterBridge(ArcBridgeService* bridge_service);
  ~ArcObbMounterBridge() override;

  // InstanceHolder<mojom::ObbMounterInstance>::Observer overrides:
  void OnInstanceReady() override;

  // mojom::ObbMounterHost overrides:
  void MountObb(const std::string& obb_file,
                const std::string& target_path,
                int32_t owner_gid,
                const MountObbCallback& callback) override;
  void UnmountObb(const std::string& target_path,
                  const UnmountObbCallback& callback) override;

  // chromeos::disks::DiskMountManager::Observer overrides.
  void OnDiskEvent(
      chromeos::disks::DiskMountManager::DiskEvent event,
      const chromeos::disks::DiskMountManager::Disk* disk) override;
  void OnDeviceEvent(chromeos::disks::DiskMountManager::DeviceEvent event,
                     const std::string& device_path) override;
  void OnMountEvent(chromeos::disks::DiskMountManager::MountEvent event,
                    chromeos::MountError error_code,
                    const chromeos::disks::DiskMountManager::MountPointInfo&
                        mount_info) override;
  void OnFormatEvent(chromeos::disks::DiskMountManager::FormatEvent event,
                     chromeos::FormatError error_code,
                     const std::string& device_path) override;

 private:
  void RemountAll();

  mojo::Binding<mojom::ObbMounterHost> binding_;

  std::future<void> remount_all_;

  DISALLOW_COPY_AND_ASSIGN(ArcObbMounterBridge);
};

}  // namespace arc

#endif  // COMPONENTS_ARC_OBB_MOUNTER_ARC_OBB_MOUNTER_BRIDGE_H_
