// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/cast/net/udp_transport_impl.h"

#include <algorithm>
#include <string>
#include <vector>

#include "base/bind.h"
#include "base/callback.h"
#include "base/macros.h"
#include "base/run_loop.h"
#include "base/test/mock_callback.h"
#include "base/test/scoped_task_environment.h"
#include "media/cast/net/cast_transport_config.h"
#include "media/cast/net/udp_packet_pipe.h"
#include "media/cast/test/utility/net_utility.h"
#include "net/base/ip_address.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace media {
namespace cast {

namespace {

class MockPacketReceiver {
 public:
  MockPacketReceiver(const base::RepeatingClosure& callback)
      : packet_callback_(callback) {}

  bool ReceivedPacket(std::unique_ptr<Packet> packet) {
    packet_ = std::string(packet->size(), '\0');
    std::copy(packet->begin(), packet->end(), packet_.begin());
    packet_callback_.Run();
    return true;
  }

  std::string packet() const { return packet_; }
  PacketReceiverCallbackWithStatus packet_receiver() {
    return base::BindRepeating(&MockPacketReceiver::ReceivedPacket,
                               base::Unretained(this));
  }

  bool StorePackets(std::unique_ptr<Packet> packet) {
    stored_packets_.push_back(std::move(packet));
    return true;
  }

  std::vector<std::unique_ptr<Packet>> stored_packets_;

 private:
  std::string packet_;
  base::RepeatingClosure packet_callback_;

  DISALLOW_COPY_AND_ASSIGN(MockPacketReceiver);
};

void SendPacket(UdpTransportImpl* transport, Packet packet) {
  base::Closure cb;
  transport->SendPacket(new base::RefCountedData<Packet>(packet), cb);
}

static void UpdateCastTransportStatus(CastTransportStatus status) {
  NOTREACHED();
}

class MockUdpTransportReceiver final : public UdpTransportReceiver {
 public:
  MockUdpTransportReceiver() {}
  ~MockUdpTransportReceiver() override {}
  void OnPacketReceived(const std::vector<uint8_t>& packet) override {
    EXPECT_GT(packet.size(), 0u);
    packets_.push_back(packet);
  }
  std::vector<Packet> packets_;

 private:
  DISALLOW_COPY_AND_ASSIGN(MockUdpTransportReceiver);
};

}  // namespace

class UdpTransportImplTest : public ::testing::Test {
 public:
  UdpTransportImplTest()
      : scoped_task_environment_(
            base::test::ScopedTaskEnvironment::MainThreadType::IO) {
    net::IPEndPoint free_local_port1 = test::GetFreeLocalPort();
    net::IPEndPoint free_local_port2 = test::GetFreeLocalPort();

    send_transport_ = std::make_unique<UdpTransportImpl>(
        nullptr, scoped_task_environment_.GetMainThreadTaskRunner(),
        free_local_port1, free_local_port2,
        base::BindRepeating(&UpdateCastTransportStatus));
    send_transport_->SetSendBufferSize(65536);

    recv_transport_ = std::make_unique<UdpTransportImpl>(
        nullptr, scoped_task_environment_.GetMainThreadTaskRunner(),
        free_local_port2, free_local_port1,
        base::BindRepeating(&UpdateCastTransportStatus));
    recv_transport_->SetSendBufferSize(65536);
  }

  ~UdpTransportImplTest() override = default;

 protected:
  base::test::ScopedTaskEnvironment scoped_task_environment_;

  std::unique_ptr<UdpTransportImpl> send_transport_;

  // A receiver side transport to receiver/send packets from/to sender.
  std::unique_ptr<UdpTransportImpl> recv_transport_;

  // TODO(xjz): Replace this with a mojo ptr.
  MockUdpTransportReceiver packet_receiver_on_sender_;

 private:
  DISALLOW_COPY_AND_ASSIGN(UdpTransportImplTest);
};

// Test the sending/receiving functions as a PacketSender.
TEST_F(UdpTransportImplTest, PacketSenderSendAndReceive) {
  Packet packet;
  packet.push_back('t');
  packet.push_back('e');
  packet.push_back('s');
  packet.push_back('t');

  base::RunLoop run_loop;
  MockPacketReceiver receiver1(run_loop.QuitClosure());
  MockPacketReceiver receiver2(
      base::BindRepeating(&SendPacket, recv_transport_.get(), packet));
  send_transport_->StartReceiving(receiver1.packet_receiver());
  recv_transport_->StartReceiving(receiver2.packet_receiver());

  base::Closure cb;
  SendPacket(send_transport_.get(), packet);
  run_loop.Run();
  EXPECT_TRUE(
      std::equal(packet.begin(), packet.end(), receiver1.packet().begin()));
  EXPECT_TRUE(
      std::equal(packet.begin(), packet.end(), receiver2.packet().begin()));
}

// Test the sending/receiving functions as a UdpTransport.

TEST_F(UdpTransportImplTest, SendingPackets) {
  send_transport_->StartReceiving(&packet_receiver_on_sender_);
  base::RepeatingClosure cb;
  MockPacketReceiver receiver(cb);
  recv_transport_->StartReceiving(base::BindRepeating(
      &MockPacketReceiver::StorePackets, base::Unretained(&receiver)));
  mojo::DataPipe data_pipe(10);
  send_transport_->StartSending(std::move(data_pipe.consumer_handle));
  UdpPacketPipeWriter writer(std::move(data_pipe.producer_handle));
  std::string data1 = "test";
  std::string data2 = "Hello!";
  Packet packet1(data1.begin(), data1.end());
  Packet packet2(data2.begin(), data2.end());

  // Writes |packet1|. Expected to be done successfully.
  base::MockCallback<base::OnceClosure> done_callback;
  EXPECT_CALL(done_callback, Run()).Times(1);
  writer.Write(new base::RefCountedData<Packet>(packet1), done_callback.Get());
  scoped_task_environment_.RunUntilIdle();
  EXPECT_EQ(1u, receiver.stored_packets_.size());

  // Writes |packet2|. Expected to be done successfully.
  base::MockCallback<base::OnceClosure> done_callback2;
  EXPECT_CALL(done_callback2, Run()).Times(1);
  writer.Write(new base::RefCountedData<Packet>(packet2), done_callback2.Get());
  scoped_task_environment_.RunUntilIdle();
  EXPECT_EQ(2u, receiver.stored_packets_.size());

  EXPECT_EQ(0, std::memcmp(packet1.data(), receiver.stored_packets_[0]->data(),
                           packet1.size()));
  EXPECT_EQ(0, std::memcmp(packet2.data(), receiver.stored_packets_[1]->data(),
                           packet2.size()));

  // Expect no packets received on sender side.
  EXPECT_TRUE(packet_receiver_on_sender_.packets_.empty());
}

TEST_F(UdpTransportImplTest, ReceivingPackets) {
  send_transport_->StartReceiving(&packet_receiver_on_sender_);
  base::RepeatingClosure cb;
  MockPacketReceiver receiver(cb);
  recv_transport_->StartReceiving(base::BindRepeating(
      &MockPacketReceiver::StorePackets, base::Unretained(&receiver)));
  std::string data1 = "test";
  std::string data2 = "Hello!";
  Packet packet1(data1.begin(), data1.end());
  Packet packet2(data2.begin(), data2.end());

  // Receiver sends |packet1| to sender. Expected to be done successfully.
  SendPacket(recv_transport_.get(), packet1);
  scoped_task_environment_.RunUntilIdle();
  EXPECT_EQ(1u, packet_receiver_on_sender_.packets_.size());

  // Receiver sends |packet2| to sender. Expected to be done successfully.
  SendPacket(recv_transport_.get(), packet2);
  scoped_task_environment_.RunUntilIdle();
  EXPECT_EQ(2u, packet_receiver_on_sender_.packets_.size());

  EXPECT_EQ(0, std::memcmp(packet1.data(),
                           packet_receiver_on_sender_.packets_[0].data(),
                           packet1.size()));
  EXPECT_EQ(0, std::memcmp(packet2.data(),
                           packet_receiver_on_sender_.packets_[1].data(),
                           packet2.size()));
  EXPECT_TRUE(receiver.stored_packets_.empty());
  packet_receiver_on_sender_.packets_.clear();
  send_transport_->StopReceiving();

  // Send one more packet to sender.
  SendPacket(recv_transport_.get(), packet1);
  scoped_task_environment_.RunUntilIdle();

  // Expect no packets received after calling StopReceiving().
  EXPECT_TRUE(packet_receiver_on_sender_.packets_.empty());
}

}  // namespace cast
}  // namespace media
