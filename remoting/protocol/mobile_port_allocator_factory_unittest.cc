// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/protocol/mobile_port_allocator_factory.h"

#include <algorithm>

#include "testing/gtest/include/gtest/gtest.h"

namespace {
void AssertNetworkInList(const rtc::NetworkManager::NetworkList& list,
                         const rtc::Network& network) {
  ASSERT_NE(list.end(), std::find(list.begin(), list.end(), &network));
}
}  // namespace

namespace remoting {
namespace protocol {

class MobilePortAllocatorFactoryTest : public testing::Test {
 public:
  MobilePortAllocatorFactoryTest();
  ~MobilePortAllocatorFactoryTest() override;

 protected:
  rtc::Network wifi0{"wifi0", "wifi0", rtc::IPAddress(), 0,
                     rtc::AdapterType::ADAPTER_TYPE_WIFI};
  rtc::Network wifi1{"wifi0", "wifi0", rtc::IPAddress(), 0,
                     rtc::AdapterType::ADAPTER_TYPE_WIFI};
  rtc::Network cellular0{"cellular0", "cellular0", rtc::IPAddress(), 0,
                         rtc::AdapterType::ADAPTER_TYPE_CELLULAR};
  rtc::Network cellular1{"cellular1", "cellular1", rtc::IPAddress(), 0,
                         rtc::AdapterType::ADAPTER_TYPE_CELLULAR};
};

MobilePortAllocatorFactoryTest::MobilePortAllocatorFactoryTest() {}
MobilePortAllocatorFactoryTest::~MobilePortAllocatorFactoryTest() {}

TEST_F(MobilePortAllocatorFactoryTest, EmptyNetworkList_NoEffect) {
  rtc::NetworkManager::NetworkList list;
  MobilePortAllocatorFactory::FilterCellularNetworksForTest(&list);
  ASSERT_EQ(0u, list.size());
}

TEST_F(MobilePortAllocatorFactoryTest, NetworkListWithOnlyWifi_NoEffect) {
  rtc::NetworkManager::NetworkList list = {&wifi0, &wifi1};
  MobilePortAllocatorFactory::FilterCellularNetworksForTest(&list);
  ASSERT_EQ(2u, list.size());
  AssertNetworkInList(list, wifi0);
  AssertNetworkInList(list, wifi1);
}

TEST_F(MobilePortAllocatorFactoryTest, NetworkListWithOnlyCellular_NoEffect) {
  rtc::NetworkManager::NetworkList list = {&cellular0, &cellular1};
  MobilePortAllocatorFactory::FilterCellularNetworksForTest(&list);
  ASSERT_EQ(2u, list.size());
  AssertNetworkInList(list, cellular0);
  AssertNetworkInList(list, cellular1);
}

TEST_F(MobilePortAllocatorFactoryTest,
       NetworkListWithWifiAndCellular_RemovesAllCellular) {
  rtc::NetworkManager::NetworkList list = {&wifi0, &cellular0, &wifi1,
                                           &cellular1};
  MobilePortAllocatorFactory::FilterCellularNetworksForTest(&list);
  ASSERT_EQ(2u, list.size());
  AssertNetworkInList(list, wifi0);
  AssertNetworkInList(list, wifi1);
}

}  // namespace protocol
}  // namespace remoting
