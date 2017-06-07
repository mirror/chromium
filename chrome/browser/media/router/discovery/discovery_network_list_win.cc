// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/media/router/discovery/discovery_network_list.h"

#include <winsock2.h>
#include <ws2tcpip.h>

#include <iphlpapi.h>  // NOLINT

#include <windot11.h>  // NOLINT
#include <wlanapi.h>   // NOLINT

#include <algorithm>
#include <utility>
#include <vector>

#include "base/memory/ptr_util.h"
#include "base/strings/string_number_conversions.h"

namespace {

void IfTable2Deleter(PMIB_IF_TABLE2 interface_table) {
  if (interface_table) {
    FreeMibTable(interface_table);
  }
}

void WlanApiDeleter(void* p) {
  if (p) {
    WlanFreeMemory(p);
  }
}

struct MAC {
  MAC() : data{} {}
  MAC(const uint8_t* src, ULONG length) : data{} { assign(src, length); }
  void assign(const uint8_t* src, ULONG length) {
    DCHECK_LE(length, 6UL);
    memcpy(data, src, length);
  }
  bool operator==(const MAC& o) const { return memcmp(o.data, data, 6) == 0; }
  uint8_t data[6];
};

std::vector<std::pair<GUID, MAC>> GetInterfaceGuidMacMap() {
  PMIB_IF_TABLE2 interface_table_raw = nullptr;
  auto result = GetIfTable2(&interface_table_raw);
  if (result != ERROR_SUCCESS) {
    DVLOG(2) << "GetIfTable2() failed";
    return {};
  }
  auto interface_table =
      std::unique_ptr<MIB_IF_TABLE2, decltype(&IfTable2Deleter)>(
          interface_table_raw, IfTable2Deleter);

  std::vector<std::pair<GUID, MAC>> guid_mac_map;
  guid_mac_map.reserve(interface_table->NumEntries);
  for (ULONG i = 0; i < interface_table->NumEntries; ++i) {
    const auto* interface_row = &interface_table->Table[i];
    guid_mac_map.push_back(
        std::make_pair(interface_row->InterfaceGuid,
                       MAC{interface_row->PhysicalAddress,
                           interface_row->PhysicalAddressLength}));
  }

  return guid_mac_map;
}

std::vector<std::pair<MAC, std::string>> GetMacSsidMap() {
  HANDLE wlan_client_handle = nullptr;
  constexpr DWORD wlan_client_version = 2;
  DWORD wlan_current_version = 0;

  auto result = WlanOpenHandle(wlan_client_version, nullptr,
                               &wlan_current_version, &wlan_client_handle);
  if (result != ERROR_SUCCESS) {
    DVLOG(2) << "Failed to open Wlan client handle";
    return {};
  }

  PWLAN_INTERFACE_INFO_LIST wlan_interface_list_raw = nullptr;
  result =
      WlanEnumInterfaces(wlan_client_handle, nullptr, &wlan_interface_list_raw);
  if (result != ERROR_SUCCESS) {
    DVLOG(2) << "Failed to enumerate wireless interfaces";
    return {};
  }
  auto wlan_interface_list =
      std::unique_ptr<WLAN_INTERFACE_INFO_LIST, decltype(&WlanApiDeleter)>(
          wlan_interface_list_raw, WlanApiDeleter);

  auto guid_mac_map = GetInterfaceGuidMacMap();

  std::vector<std::pair<MAC, std::string>> mac_ssid_map;
  for (DWORD i = 0; i < wlan_interface_list->dwNumberOfItems; ++i) {
    const auto* interface_info = &wlan_interface_list->InterfaceInfo[i];
    const auto mac_entry = std::find_if(
        guid_mac_map.begin(), guid_mac_map.end(),
        [interface_info](const std::pair<GUID, MAC>& guid_mac_entry) {
          return guid_mac_entry.first == interface_info->InterfaceGuid;
        });
    if (mac_entry == guid_mac_map.end()) {
      continue;
    }
    const auto& interface_mac = mac_entry->second;

    WLAN_CONNECTION_ATTRIBUTES* connection_info_raw = nullptr;
    DWORD connection_info_size = 0;
    result = WlanQueryInterface(
        wlan_client_handle, &interface_info->InterfaceGuid,
        wlan_intf_opcode_current_connection, nullptr, &connection_info_size,
        reinterpret_cast<void**>(&connection_info_raw), nullptr);
    if (result != ERROR_SUCCESS) {
      // We can't get the SSID for this interface so its network ID will
      // fall back to its MAC address below.
      DVLOG(2) << "Failed to get wireless connection info";
      continue;
    }
    auto connection_info =
        std::unique_ptr<WLAN_CONNECTION_ATTRIBUTES, decltype(&WlanApiDeleter)>(
            connection_info_raw, WlanApiDeleter);
    const auto* ssid = &connection_info->wlanAssociationAttributes.dot11Ssid;
    mac_ssid_map.push_back(std::make_pair(
        interface_mac,
        std::string(reinterpret_cast<const char*>(&ssid->ucSSID[0]),
                    ssid->uSSIDLength)));
  }
  return mac_ssid_map;
}

}  // namespace

std::vector<DiscoveryNetworkInfo> GetDiscoveryNetworkInfoList() {
  // Max number of times to retry GetAdaptersAddresses due to
  // ERROR_BUFFER_OVERFLOW. If GetAdaptersAddresses returns this indefinitely
  // due to an unforseen reason, we don't want to be stuck in an endless loop.
  static constexpr int kMaxGetAdaptersAddressTries = 10;
  // Use an initial buffer size of 15KB, as recommended by MSDN. See:
  // https://msdn.microsoft.com/en-us/library/windows/desktop/aa365915(v=vs.85).aspx
  static constexpr int kInitialBufferSize = 15000;
  constexpr ULONG address_flags =
      GAA_FLAG_SKIP_UNICAST | GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST |
      GAA_FLAG_SKIP_DNS_SERVER;

  ULONG addresses_buffer_size = kInitialBufferSize;
  char initial_buf[kInitialBufferSize];
  std::unique_ptr<char[]> addresses_buffer;

  PIP_ADAPTER_ADDRESSES adapter_addresses =
      reinterpret_cast<PIP_ADAPTER_ADDRESSES>(initial_buf);
  auto result = GetAdaptersAddresses(AF_UNSPEC, address_flags, nullptr,
                                     adapter_addresses, &addresses_buffer_size);

  for (int i = 1;
       result == ERROR_BUFFER_OVERFLOW && i < kMaxGetAdaptersAddressTries;
       ++i) {
    addresses_buffer.reset(new char[addresses_buffer_size]);
    adapter_addresses =
        reinterpret_cast<PIP_ADAPTER_ADDRESSES>(addresses_buffer.get());
    result = GetAdaptersAddresses(AF_UNSPEC, address_flags, nullptr,
                                  adapter_addresses, &addresses_buffer_size);
  }

  if (result != NO_ERROR) {
    return std::vector<DiscoveryNetworkInfo>();
  }

  std::vector<DiscoveryNetworkInfo> network_ids;
  auto mac_ssid_map = GetMacSsidMap();
  for (const IP_ADAPTER_ADDRESSES* current_adapter = adapter_addresses;
       current_adapter != nullptr; current_adapter = current_adapter->Next) {
    if (current_adapter->OperStatus != IfOperStatusUp ||
        (current_adapter->IfType != IF_TYPE_ETHERNET_CSMACD &&
         current_adapter->IfType != IF_TYPE_IEEE80211)) {
      continue;
    }
    std::string name(current_adapter->AdapterName);
    // We have to use a slightly roundabout way to get the SSID for each
    // adapter:
    // - Enumerate wifi devices to get list of interface GUIDs.
    // - Enumerate interfaces to get interface GUID -> physical address map.
    // - Map interface GUIDs to SSID.
    // - Use GUID -> MAC map to do MAC -> interface GUID  -> SSID.
    // Although it's theoretically possible to have multiple interfaces per
    // adapter, most wireless cards don't actually allow multiple
    // managed-mode interfaces.  However, in the event that there really
    // are multiple interfaces per adapter (i.e. physical address), we will
    // simply use the SSID of the first match.  Honestly, though, I don't
    // know for sure how Windows would handle this case since it's kind of fuzzy
    // between its use of "adapter" and "interface".
    if (current_adapter->IfType == IF_TYPE_IEEE80211) {
      MAC adapter_mac(current_adapter->PhysicalAddress,
                      current_adapter->PhysicalAddressLength);
      const auto ssid_entry = std::find_if(
          mac_ssid_map.begin(), mac_ssid_map.end(),
          [&adapter_mac](const std::pair<MAC, std::string>& mac_ssid_entry) {
            return mac_ssid_entry.first == adapter_mac;
          });
      if (ssid_entry != mac_ssid_map.end()) {
        network_ids.push_back({name, ssid_entry->second});
        continue;
      }
    }
    network_ids.push_back(
        {name, base::HexEncode(current_adapter->PhysicalAddress,
                               current_adapter->PhysicalAddressLength)});
  }

  StableSortDiscoveryNetworkInfo(network_ids.begin(), network_ids.end());

  return network_ids;
}
