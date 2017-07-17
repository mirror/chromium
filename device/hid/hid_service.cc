// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/hid/hid_service.h"

#include "base/at_exit.h"
#include "base/atomic_sequence_num.h"
#include "base/bind.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "base/stl_util.h"
#include "base/threading/thread_task_runner_handle.h"
#include "build/build_config.h"
#include "components/device_event_log/device_event_log.h"

#if defined(OS_LINUX) && defined(USE_UDEV)
#include "device/hid/hid_service_linux.h"
#elif defined(OS_MACOSX)
#include "device/hid/hid_service_mac.h"
#elif defined(OS_WIN)
#include "device/hid/hid_service_win.h"
#endif

namespace device {

namespace {
base::StaticAtomicSequenceNumber device_id_generator;
}  // namespace

void HidService::Observer::OnDeviceAdded(
    scoped_refptr<HidDeviceInfo> device_info) {
}

void HidService::Observer::OnDeviceRemoved(
    scoped_refptr<HidDeviceInfo> device_info) {
}

void HidService::Observer::OnDeviceRemovedCleanup(
    scoped_refptr<HidDeviceInfo> device_info) {
}

// static
constexpr base::TaskTraits HidService::kBlockingTaskTraits;

std::unique_ptr<HidService> HidService::Create() {
#if defined(OS_LINUX) && defined(USE_UDEV)
  return base::WrapUnique(new HidServiceLinux());
#elif defined(OS_MACOSX)
  return base::WrapUnique(new HidServiceMac());
#elif defined(OS_WIN)
  return base::WrapUnique(new HidServiceWin());
#else
  return nullptr;
#endif
}

void HidService::GetDevices(const GetDevicesCallback& callback) {
  DCHECK(thread_checker_.CalledOnValidThread());
  if (enumeration_ready_) {
    std::vector<scoped_refptr<HidDeviceInfo>> devices;
    for (const auto& map_entry : devices_) {
      devices.push_back(map_entry.second);
    }
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE, base::Bind(callback, devices));
  } else {
    pending_enumerations_.push_back(callback);
  }
}

void HidService::AddObserver(HidService::Observer* observer) {
  observer_list_.AddObserver(observer);
}

void HidService::RemoveObserver(HidService::Observer* observer) {
  observer_list_.RemoveObserver(observer);
}

// Fills in the device info struct of the given device_id.
scoped_refptr<HidDeviceInfo> HidService::GetDeviceInfo(
    const HidDeviceId& device_id) const {
  DCHECK(thread_checker_.CalledOnValidThread());
  DeviceMap::const_iterator it = devices_.find(device_id);
  if (it == devices_.end()) {
    return nullptr;
  }
  return it->second;
}

HidService::HidService() {
  // kInvalidHidDeviceId = 0, so let the HidDeviceID start from 1.
  device_id_generator.GetNext();
}

HidService::~HidService() {
  DCHECK(thread_checker_.CalledOnValidThread());
}

void HidService::AddDevice(scoped_refptr<HidDeviceInfo> device_info,
                           const HidPlatformDeviceId& platform_device_id) {
  DCHECK(thread_checker_.CalledOnValidThread());

  HidDeviceId device_id = AddDeviceIdMapping(platform_device_id);
  device_info->set_device_id(device_id);

  if (!base::ContainsKey(devices_, device_id)) {
    devices_[device_id] = device_info;

    HID_LOG(USER) << "HID device "
                  << (enumeration_ready_ ? "added" : "detected")
                  << ": vendorId=" << device_info->vendor_id()
                  << ", productId=" << device_info->product_id() << ", name='"
                  << device_info->product_name() << "', serial='"
                  << device_info->serial_number() << "', deviceId='"
                  << platform_device_id << "'";

    if (enumeration_ready_) {
      for (auto& observer : observer_list_)
        observer.OnDeviceAdded(device_info);
    }
  }
}

void HidService::RemoveDevice(const HidPlatformDeviceId& platform_device_id) {
  DCHECK(thread_checker_.CalledOnValidThread());
  HidDeviceId device_id = FindDeviceIdByPlatformDeviceId(platform_device_id);
  if (device_id == kInvalidHidDeviceId)
    return;
  device_id_map_.erase(device_id);

  DeviceMap::iterator it = devices_.find(device_id);
  if (it != devices_.end()) {
    HID_LOG(USER) << "HID device removed: deviceId='" << platform_device_id
                  << "'";

    scoped_refptr<HidDeviceInfo> device = it->second;
    if (enumeration_ready_) {
      for (auto& observer : observer_list_)
        observer.OnDeviceRemoved(device);
    }
    devices_.erase(it);
    if (enumeration_ready_) {
      for (auto& observer : observer_list_)
        observer.OnDeviceRemovedCleanup(device);
    }
  }
}

void HidService::FirstEnumerationComplete() {
  enumeration_ready_ = true;

  if (!pending_enumerations_.empty()) {
    std::vector<scoped_refptr<HidDeviceInfo>> devices;
    for (const auto& map_entry : devices_) {
      devices.push_back(map_entry.second);
    }

    for (const GetDevicesCallback& callback : pending_enumerations_) {
      callback.Run(devices);
    }
    pending_enumerations_.clear();
  }
}

HidDeviceId HidService::AddDeviceIdMapping(
    const HidPlatformDeviceId& platform_device_id) {
  HidDeviceId device_id = FindDeviceIdByPlatformDeviceId(platform_device_id);
  // Only add the |platform_device_id| into map if it is not found.
  if (device_id == kInvalidHidDeviceId) {
    device_id = device_id_generator.GetNext();
    device_id_map_[device_id] = platform_device_id;
  }
  return device_id;
}

HidPlatformDeviceId HidService::FindPlatformDeviceIdByDeviceId(
    const HidDeviceId& device_id) {
  auto it = device_id_map_.find(device_id);
  return it != device_id_map_.end() ? it->second : kInvalidHidPlatformDeviceId;
}

HidDeviceId HidService::FindDeviceIdByPlatformDeviceId(
    const HidPlatformDeviceId& platform_device_id) {
  for (auto it = device_id_map_.begin(); it != device_id_map_.end(); ++it) {
    if (it->second == platform_device_id) {
      return it->first;
    }
  }
  return kInvalidHidDeviceId;
}

}  // namespace device
