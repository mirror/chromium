// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/protocol/mobile_network_manager.h"

#include <algorithm>
#include <memory>

#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/webrtc/rtc_base/ipaddress.h"

namespace {

void AddIp(rtc::Network* network, const std::string& ip_string) {
  rtc::IPAddress ip;
  rtc::IPFromString(ip_string, &ip);
  network->AddIP(ip);
}

}  // namespace

namespace remoting {
namespace protocol {

class MobileNetworkManagerTest : public testing::Test {
 public:
  void SetUp() override;

 protected:
  void VerifyNetworkFilter(
      const rtc::NetworkManager::NetworkList& input,
      const rtc::NetworkManager::NetworkList& expected_output) const;

  rtc::Network wifi_1{"en0", "en0", rtc::IPAddress(), 0,
                      rtc::AdapterType::ADAPTER_TYPE_WIFI};
  rtc::Network wifi_2{"en0", "en0", rtc::IPAddress(), 0,
                      rtc::AdapterType::ADAPTER_TYPE_WIFI};
  rtc::Network wifi_link_local_1{"en2", "en2", rtc::IPAddress(), 0,
                                 rtc::AdapterType::ADAPTER_TYPE_WIFI};
  rtc::Network wifi_link_local_2{"en2", "en2", rtc::IPAddress(), 0,
                                 rtc::AdapterType::ADAPTER_TYPE_WIFI};
  rtc::Network cellular_1{"pdp_ip0", "pdp_ip0", rtc::IPAddress(), 0,
                          rtc::AdapterType::ADAPTER_TYPE_CELLULAR};
  rtc::Network cellular_2{"pdp_ip0", "pdp_ip0", rtc::IPAddress(), 0,
                          rtc::AdapterType::ADAPTER_TYPE_CELLULAR};
};

void MobileNetworkManagerTest::SetUp() {
  AddIp(&wifi_1, "192.168.0.1");
  AddIp(&wifi_2, "2001:0db8::1");

  AddIp(&wifi_link_local_1, "169.254.0.1");
  AddIp(&wifi_link_local_2, "fe80::1");

  AddIp(&cellular_1, "192.168.0.2");
  AddIp(&cellular_2, "2001:0db8::2");
}

void MobileNetworkManagerTest::VerifyNetworkFilter(
    const rtc::NetworkManager::NetworkList& input,
    const rtc::NetworkManager::NetworkList& expected_output) const {
  rtc::NetworkManager::NetworkList list(input);
  MobileNetworkManager::FilterCellularNetworks(&list);
  ASSERT_EQ(expected_output.size(), list.size());
  for (const rtc::Network* network : expected_output) {
    ASSERT_NE(list.end(), std::find(list.begin(), list.end(), network));
  }
}

// Test cases

TEST_F(MobileNetworkManagerTest, EmptyNetworkList_NoEffect) {
  VerifyNetworkFilter({}, {});
}

TEST_F(MobileNetworkManagerTest,
       NetworkListWithOnlyWifiAndLinkLocalWifi_NoEffect) {
  VerifyNetworkFilter(
      {&wifi_1, &wifi_2, &wifi_link_local_1, &wifi_link_local_2},
      {&wifi_1, &wifi_2, &wifi_link_local_1, &wifi_link_local_2});
}

TEST_F(MobileNetworkManagerTest, NetworkListWithOnlyCellular_NoEffect) {
  VerifyNetworkFilter({&cellular_1, &cellular_2}, {&cellular_1, &cellular_2});
}

TEST_F(MobileNetworkManagerTest,
       NetworkListWithLinkLocalWifiAndCellular_NoEffect) {
  VerifyNetworkFilter(
      {&wifi_link_local_1, &cellular_1, &wifi_link_local_2, &cellular_2},
      {&wifi_link_local_1, &cellular_1, &wifi_link_local_2, &cellular_2});
}

TEST_F(MobileNetworkManagerTest,
       NetworkListWithWifiAndCellular_RemovesAllCellular) {
  VerifyNetworkFilter({&wifi_1, &cellular_1, &wifi_2, &cellular_2},
                      {&wifi_1, &wifi_2});
}

TEST_F(MobileNetworkManagerTest,
       NetworkListWithWifiAndLinkLocalWifiAndCellular_RemovesAllCellular) {
  VerifyNetworkFilter(
      {&wifi_link_local_1, &wifi_1, &cellular_1, &wifi_link_local_2, &wifi_2,
       &cellular_2},
      {&wifi_link_local_1, &wifi_1, &wifi_link_local_2, &wifi_2});
}

}  // namespace protocol
}  // namespace remoting
