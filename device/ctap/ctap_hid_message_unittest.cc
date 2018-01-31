// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/ctap/ctap_hid_message.h"

#include "base/memory/ptr_util.h"
#include "base/numerics/safe_conversions.h"
#include "device/ctap/ctap_constants.h"
#include "device/ctap/ctap_hid_packet.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace device {

// Packets should be 64 bytes + 1 report ID byte
TEST(CTAPHidMessageTest, TestPacketSize) {
  uint32_t channel_id = 0x05060708;
  std::vector<uint8_t> data;

  auto init_packet = std::make_unique<CTAPHidInitPacket>(
      channel_id, CTAPHIDDeviceCommand::kCtapHidInit, data, data.size());
  EXPECT_EQ(64u, init_packet->GetSerializedData().size());

  auto continuation_packet =
      std::make_unique<CTAPHidContinuationPacket>(channel_id, 0, data);
  EXPECT_EQ(64u, continuation_packet->GetSerializedData().size());
}

/*
 * U2f Init Packets are of the format:
 * Byte 0:    0
 * Byte 1-4:  Channel ID
 * Byte 5:    Command byte
 * Byte 6-7:  Big Endian size of data
 * Byte 8-n:  Data block
 *
 * Remaining buffer is padded with 0
 */
TEST(CTAPHidMessageTest, TestPacketData) {
  uint32_t channel_id = 0xF5060708;
  std::vector<uint8_t> data{10, 11};
  CTAPHIDDeviceCommand cmd = CTAPHIDDeviceCommand::kCtapHidWink;
  auto init_packet =
      std::make_unique<CTAPHidInitPacket>(channel_id, cmd, data, data.size());
  size_t index = 0;

  std::vector<uint8_t> serialized = init_packet->GetSerializedData();
  EXPECT_EQ((channel_id >> 24) & 0xff, serialized[index++]);
  EXPECT_EQ((channel_id >> 16) & 0xff, serialized[index++]);
  EXPECT_EQ((channel_id >> 8) & 0xff, serialized[index++]);
  EXPECT_EQ(channel_id & 0xff, serialized[index++]);
  EXPECT_EQ(base::strict_cast<uint8_t>(cmd), serialized[index++]);

  EXPECT_EQ(data.size() >> 8, serialized[index++]);
  EXPECT_EQ(data.size() & 0xff, serialized[index++]);
  EXPECT_EQ(data[0], serialized[index++]);
  EXPECT_EQ(data[1], serialized[index++]);
  for (; index < serialized.size(); index++)
    EXPECT_EQ(0, serialized[index]) << "mismatch at index " << index;
}

TEST(CTAPHidMessageTest, TestPacketConstructors) {
  uint32_t channel_id = 0x05060708;
  std::vector<uint8_t> data{10, 11};
  CTAPHIDDeviceCommand cmd = CTAPHIDDeviceCommand::kCtapHidWink;
  auto orig_packet =
      std::make_unique<CTAPHidInitPacket>(channel_id, cmd, data, data.size());

  size_t payload_length = static_cast<size_t>(orig_packet->payload_length());
  std::vector<uint8_t> orig_data = orig_packet->GetSerializedData();

  auto reconstructed_packet =
      CTAPHidInitPacket::CreateFromSerializedData(orig_data, &payload_length);
  EXPECT_EQ(orig_packet->command(), reconstructed_packet->command());
  EXPECT_EQ(orig_packet->payload_length(),
            reconstructed_packet->payload_length());
  EXPECT_THAT(orig_packet->GetPacketPayload(),
              testing::ContainerEq(reconstructed_packet->GetPacketPayload()));

  EXPECT_EQ(channel_id, reconstructed_packet->channel_id());

  ASSERT_EQ(orig_packet->GetSerializedData().size(),
            reconstructed_packet->GetSerializedData().size());
  for (size_t index = 0; index < orig_packet->GetSerializedData().size();
       ++index) {
    EXPECT_EQ(orig_packet->GetSerializedData()[index],
              reconstructed_packet->GetSerializedData()[index])
        << "mismatch at index " << index;
  }
}

TEST(CTAPHidMessageTest, TestMaxLengthPacketConstructors) {
  uint32_t channel_id = 0xAAABACAD;
  std::vector<uint8_t> data;
  for (size_t i = 0; i < kMaxMessageSize; ++i)
    data.push_back(static_cast<uint8_t>(i % 0xff));

  auto orig_msg = CTAPHidMessage::Create(
      channel_id, CTAPHIDDeviceCommand::kCtapHidMsg, data);
  ASSERT_TRUE(orig_msg);

  auto it = orig_msg->begin();
  auto msg_data = (*it)->GetSerializedData();
  auto new_msg = CTAPHidMessage::CreateFromSerializedData(msg_data);
  it++;

  for (; it != orig_msg->end(); ++it) {
    msg_data = (*it)->GetSerializedData();
    new_msg->AddContinuationPacket(msg_data);
  }

  auto orig_it = orig_msg->begin();
  auto new_it = new_msg->begin();

  for (; orig_it != orig_msg->end() && new_it != new_msg->end();
       ++orig_it, ++new_it) {
    EXPECT_THAT((*orig_it)->GetPacketPayload(),
                testing::ContainerEq((*new_it)->GetPacketPayload()));

    EXPECT_EQ((*orig_it)->channel_id(), (*new_it)->channel_id());

    ASSERT_EQ((*orig_it)->GetSerializedData().size(),
              (*new_it)->GetSerializedData().size());
    for (size_t index = 0; index < (*orig_it)->GetSerializedData().size();
         ++index) {
      EXPECT_EQ((*orig_it)->GetSerializedData()[index],
                (*new_it)->GetSerializedData()[index])
          << "mismatch at index " << index;
    }
  }
}

TEST(CTAPHidMessageTest, TestMessagePartitoning) {
  uint32_t channel_id = 0x01010203;
  std::vector<uint8_t> data(kInitPacketDataSize + 1);
  auto two_packet_message = CTAPHidMessage::Create(
      channel_id, CTAPHIDDeviceCommand::kCtapHidPing, data);
  ASSERT_TRUE(two_packet_message);
  EXPECT_EQ(2U, two_packet_message->NumPackets());

  data.resize(kInitPacketDataSize);
  auto one_packet_message = CTAPHidMessage::Create(
      channel_id, CTAPHIDDeviceCommand::kCtapHidPing, data);
  ASSERT_TRUE(one_packet_message);
  EXPECT_EQ(1U, one_packet_message->NumPackets());

  data.resize(kInitPacketDataSize + kContinuationPacketDataSize + 1);
  auto three_packet_message = CTAPHidMessage::Create(
      channel_id, CTAPHIDDeviceCommand::kCtapHidPing, data);
  ASSERT_TRUE(three_packet_message);
  EXPECT_EQ(3U, three_packet_message->NumPackets());
}

TEST(CTAPHidMessageTest, TestMaxSize) {
  uint32_t channel_id = 0x00010203;
  std::vector<uint8_t> data(kMaxMessageSize + 1);
  auto oversize_message = CTAPHidMessage::Create(
      channel_id, CTAPHIDDeviceCommand::kCtapHidPing, data);
  EXPECT_EQ(nullptr, oversize_message);
}

TEST(CTAPHidMessageTest, TestDeconstruct) {
  uint32_t channel_id = 0x0A0B0C0D;
  std::vector<uint8_t> data(kMaxMessageSize, 0x7F);
  auto filled_message = CTAPHidMessage::Create(
      channel_id, CTAPHIDDeviceCommand::kCtapHidPing, data);
  ASSERT_TRUE(filled_message);
  EXPECT_THAT(data, testing::ContainerEq(filled_message->GetMessagePayload()));
}

TEST(CTAPHidMessageTest, TestDeserialize) {
  uint32_t channel_id = 0x0A0B0C0D;
  std::vector<uint8_t> data(kMaxMessageSize);

  auto orig_message = CTAPHidMessage::Create(
      channel_id, CTAPHIDDeviceCommand::kCtapHidPing, data);
  ASSERT_TRUE(orig_message);

  std::list<std::vector<uint8_t>> orig_list;
  auto buf = orig_message->PopNextPacket();
  orig_list.push_back(buf);

  auto new_message = CTAPHidMessage::CreateFromSerializedData(buf);
  while (!new_message->MessageComplete()) {
    buf = orig_message->PopNextPacket();
    orig_list.push_back(buf);
    new_message->AddContinuationPacket(buf);
  }

  while (!(buf = new_message->PopNextPacket()).empty()) {
    ASSERT_EQ(buf.size(), orig_list.front().size());
    EXPECT_EQ(0, memcmp(buf.data(), orig_list.front().data(), buf.size()));
    orig_list.pop_front();
  }
}
}  // namespace device
