// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/mirroring/udp_transport_host_impl.h"

#include "base/callback.h"
#include "base/test/mock_callback.h"
#include "base/test/scoped_task_environment.h"
#include "media/cast/net/cast_transport_defines.h"
#include "media/cast/net/udp_packet_pipe.h"
#include "media/cast/net/udp_transport.h"
#include "media/cast/test/utility/net_utility.h"
#include "net/base/ip_address.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace media {

using cast::Packet;

namespace {

class MockPacketReceiver {
 public:
  MockPacketReceiver() {}
  ~MockPacketReceiver() {}

  bool ReceivedPacket(std::unique_ptr<Packet> packet) {
    packets_.push_back(std::move(packet));
    return true;
  }

  std::vector<std::unique_ptr<Packet>> packets_;

 private:
  DISALLOW_COPY_AND_ASSIGN(MockPacketReceiver);
};

class MockUdpTransportClient final : public UdpTransportClient {
 public:
  MockUdpTransportClient() {}
  ~MockUdpTransportClient() override {}
  void OnPacketReceived(const std::vector<uint8_t>& packet) override {
    EXPECT_GT(packet.size(), 0u);
    packets_.push_back(packet);
  }
  std::vector<Packet> packets_;

 private:
  DISALLOW_COPY_AND_ASSIGN(MockUdpTransportClient);
};

static void UpdateCastTransportStatus(cast::CastTransportStatus status) {}

}  // namespace

class UdpTransportHostImplTest : public ::testing::Test {
 public:
  UdpTransportHostImplTest()
      : scoped_task_environment_(
            base::test::ScopedTaskEnvironment::MainThreadType::IO) {
    net::IPEndPoint free_local_port1 = cast::test::GetFreeLocalPort();
    net::IPEndPoint free_local_port2 = cast::test::GetFreeLocalPort();

    auto transport = std::make_unique<cast::UdpTransport>(
        nullptr, scoped_task_environment_.GetMainThreadTaskRunner(),
        free_local_port1, free_local_port2,
        base::BindRepeating(&UpdateCastTransportStatus));
    transport->SetSendBufferSize(65536);
    transport_host_.reset(new UdpTransportHostImpl(std::move(transport)));
    transport_host_->StartReceiving(&client_);

    receiver_transport_ = std::make_unique<cast::UdpTransport>(
        nullptr, scoped_task_environment_.GetMainThreadTaskRunner(),
        free_local_port2, free_local_port1,
        base::BindRepeating(&UpdateCastTransportStatus));
    receiver_transport_->SetSendBufferSize(65536);
    receiver_transport_->StartReceiving(base::BindRepeating(
        &MockPacketReceiver::ReceivedPacket, base::Unretained(&receiver_)));
  }

  ~UdpTransportHostImplTest() override = default;

 protected:
  base::test::ScopedTaskEnvironment scoped_task_environment_;

  std::unique_ptr<UdpTransportHostImpl> transport_host_;

  // A receiver side transport to receiver/send packets from/to sender.
  std::unique_ptr<cast::UdpTransport> receiver_transport_;

  // Stores the packets received on sender side.
  MockUdpTransportClient client_;

  // Stores the packets received on receiver side.
  MockPacketReceiver receiver_;

 private:
  DISALLOW_COPY_AND_ASSIGN(UdpTransportHostImplTest);
};

TEST_F(UdpTransportHostImplTest, SendingPackets) {
  mojo::DataPipe data_pipe(10);
  transport_host_->StartSending(std::move(data_pipe.consumer_handle));
  cast::UdpPacketPipeWriter writer(std::move(data_pipe.producer_handle));
  std::string data1 = "test";
  std::string data2 = "Hello!";
  cast::Packet packet1(data1.begin(), data1.end());
  cast::Packet packet2(data2.begin(), data2.end());
  // Writes |packet1|. Expected to be done successfully.
  base::MockCallback<base::OnceClosure> done_callback;
  EXPECT_CALL(done_callback, Run()).Times(1);
  writer.Write(new base::RefCountedData<Packet>(packet1), done_callback.Get());
  scoped_task_environment_.RunUntilIdle();

  // Writes |packet2|. Expected to be done successfully.
  base::MockCallback<base::OnceClosure> done_callback2;
  EXPECT_CALL(done_callback2, Run()).Times(1);
  writer.Write(new base::RefCountedData<Packet>(packet2), done_callback2.Get());
  scoped_task_environment_.RunUntilIdle();

  // Both packets are expected to be sent out.
  EXPECT_EQ(2u, receiver_.packets_.size());
  EXPECT_EQ(0, std::memcmp(packet1.data(), receiver_.packets_[0]->data(),
                           packet1.size()));
  EXPECT_EQ(0, std::memcmp(packet2.data(), receiver_.packets_[1]->data(),
                           packet2.size()));
  // Expect no packets received on sender side.
  EXPECT_TRUE(client_.packets_.empty());
}

TEST_F(UdpTransportHostImplTest, ReceivingPackets) {
  std::string data1 = "test";
  std::string data2 = "Hello!";
  cast::Packet packet1(data1.begin(), data1.end());
  cast::Packet packet2(data2.begin(), data2.end());
  base::RepeatingClosure cb;
  receiver_transport_->SendPacket(new base::RefCountedData<Packet>(packet1),
                                  cb);
  scoped_task_environment_.RunUntilIdle();
  receiver_transport_->SendPacket(new base::RefCountedData<Packet>(packet2),
                                  cb);
  scoped_task_environment_.RunUntilIdle();
  // Both packets are expected to be received on sender side.
  EXPECT_EQ(2u, client_.packets_.size());
  EXPECT_EQ(0, std::memcmp(packet1.data(), client_.packets_[0].data(),
                           packet1.size()));
  EXPECT_EQ(0, std::memcmp(packet2.data(), client_.packets_[1].data(),
                           packet2.size()));
  EXPECT_TRUE(receiver_.packets_.empty());
  client_.packets_.clear();
  transport_host_->StopReceiving();
  // Send one more packet to sender.
  receiver_transport_->SendPacket(new base::RefCountedData<Packet>(packet1),
                                  cb);
  scoped_task_environment_.RunUntilIdle();
  // Expect no packets received after calling StopReceiving().
  EXPECT_TRUE(client_.packets_.empty());
}

}  // namespace media
