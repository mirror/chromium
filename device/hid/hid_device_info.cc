// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/hid/hid_device_info.h"

#include "base/atomic_sequence_num.h"
#include "build/build_config.h"
#include "device/hid/hid_report_descriptor.h"

namespace device {

namespace {
base::StaticAtomicSequenceNumber g_unique_key;

// To unify the PlatformHidDeviceId in different OS, The PlatformHidDeviceId
// is saved in a std::map. The key is used as identification of that device.
std::map<HidDeviceId, PlatformHidDeviceId> g_device_id_mapping_;
}  // namespace

#if !defined(OS_MACOSX)
const char kInvalidPlatformHidDeviceId[] = "";
#endif

HidDeviceInfo::HidDeviceInfo(const PlatformHidDeviceId& platform_hid_device_id,
                             uint16_t vendor_id,
                             uint16_t product_id,
                             const std::string& product_name,
                             const std::string& serial_number,
                             HidBusType bus_type,
                             const std::vector<uint8_t> report_descriptor)
    : device_id_(FindDeviceIdMappingInMap(platform_hid_device_id)),
      vendor_id_(vendor_id),
      product_id_(product_id),
      product_name_(product_name),
      serial_number_(serial_number),
      bus_type_(bus_type),
      report_descriptor_(report_descriptor) {
  if (device_id_ == kInvalidHidDeviceId) {
    device_id_ = g_unique_key.GetNext();
    g_device_id_mapping_[device_id_] = platform_hid_device_id;
  }
  HidReportDescriptor descriptor_parser(report_descriptor_);
  descriptor_parser.GetDetails(
      &collections_, &has_report_id_, &max_input_report_size_,
      &max_output_report_size_, &max_feature_report_size_);
}

HidDeviceInfo::HidDeviceInfo(const PlatformHidDeviceId& platform_hid_device_id,
                             uint16_t vendor_id,
                             uint16_t product_id,
                             const std::string& product_name,
                             const std::string& serial_number,
                             HidBusType bus_type,
                             const HidCollectionInfo& collection,
                             size_t max_input_report_size,
                             size_t max_output_report_size,
                             size_t max_feature_report_size)
    : device_id_(FindDeviceIdMappingInMap(platform_hid_device_id)),
      vendor_id_(vendor_id),
      product_id_(product_id),
      product_name_(product_name),
      serial_number_(serial_number),
      bus_type_(bus_type),
      max_input_report_size_(max_input_report_size),
      max_output_report_size_(max_output_report_size),
      max_feature_report_size_(max_feature_report_size) {
  if (device_id_ == kInvalidHidDeviceId) {
    device_id_ = g_unique_key.GetNext();
    g_device_id_mapping_[device_id_] = platform_hid_device_id;
  }
  collections_.push_back(collection);
  has_report_id_ = !collection.report_ids.empty();
}

HidDeviceInfo::~HidDeviceInfo() {}

// static
PlatformHidDeviceId HidDeviceInfo::GetPlatformHidDeviceIdFromMap(
    const HidDeviceId& device_id) {
  auto it = g_device_id_mapping_.find(device_id);
  return it != g_device_id_mapping_.end() ? it->second
                                          : kInvalidPlatformHidDeviceId;
}

// static
HidDeviceId HidDeviceInfo::FindDeviceIdMappingInMap(
    const PlatformHidDeviceId& platform_hid_device_id) {
  for (auto it = g_device_id_mapping_.begin(); it != g_device_id_mapping_.end();
       ++it) {
    if (it->second == platform_hid_device_id) {
      return it->first;
    }
  }
  return kInvalidHidDeviceId;
}

// static
HidDeviceId HidDeviceInfo::RemoveDeviceIdMappingFromMap(
    const PlatformHidDeviceId& platform_hid_device_id) {
  HidDeviceId device_id = FindDeviceIdMappingInMap(platform_hid_device_id);
  if (device_id != kInvalidHidDeviceId)
    g_device_id_mapping_.erase(device_id);

  return device_id;
}

}  // namespace device
