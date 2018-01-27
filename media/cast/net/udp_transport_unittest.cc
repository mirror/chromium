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
#include "media/cast/net/mojo_udp_transport_client.h"
#include "media/cast/net/udp_packet_pipe.h"
#include "media/cast/test/utility/net_utility.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "net/base/ip_address.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace media {
namespace cast {

namespace {

class MockPacketReceiver final : public mojom::UdpTransportReceiver {
 public:
  MockPacketReceiver(const base::RepeatingClosure& callback)
      : packet_callback_(callback), binding_(this) {}

  bool ReceivedPacket(std::unique_ptr<Packet> packet) {
    packet_ = std::move(packet);
    packet_callback_.Run();
    return true;
  }

  // mojom::UdpTransportReceiver implementation.
  void OnPacketReceived(const std::vector<uint8_t>& packet) override {
    EXPECT_GT(packet.size(), 0u);
    packet_.reset(new Packet(packet));
    packet_callback_.Run();
  }

  PacketReceiverCallbackWithStatus packet_receiver() {
    return base::BindRepeating(&MockPacketReceiver::ReceivedPacket,
                               base::Unretained(this));
  }

  void BindToRequest(mojom::UdpTransportReceiverRequest request) {
    binding_.Bind(std::move(request));
  }

  std::unique_ptr<Packet> TakePacket() { return std::move(packet_); }

 private:
  base::RepeatingClosure packet_callback_;
  mojo::Binding<mojom::UdpTransportReceiver> binding_;
  std::unique_ptr<Packet> packet_;

  DISALLOW_COPY_AND_ASSIGN(MockPacketReceiver);
};

void SendPacket(PacketTransport* transport, Packet packet) {
  transport->SendPacket(new base::RefCountedData<Packet>(packet),
                        base::BindRepeating([]() {}));
}

static void UpdateCastTransportStatus(CastTransportStatus status) {
  NOTREACHED();
}

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

 private:
  DISALLOW_COPY_AND_ASSIGN(UdpTransportImplTest);
};

// Test the sending/receiving as a PacketSender.
TEST_F(UdpTransportImplTest, PacketSenderSendAndReceive) {
  std::string data = "Test";
  Packet packet(data.begin(), data.end());

  base::RunLoop run_loop;
  MockPacketReceiver packet_receiver_on_sender(run_loop.QuitClosure());
  MockPacketReceiver packet_receiver_on_receiver(
      base::BindRepeating(&SendPacket, recv_transport_.get(), packet));
  send_transport_->StartReceiving(packet_receiver_on_sender.packet_receiver());
  recv_transport_->StartReceiving(
      packet_receiver_on_receiver.packet_receiver());

  SendPacket(send_transport_.get(), packet);
  run_loop.Run();
  std::unique_ptr<Packet> received_packet =
      packet_receiver_on_sender.TakePacket();
  EXPECT_TRUE(received_packet);
  EXPECT_TRUE(
      std::equal(packet.begin(), packet.end(), received_packet->begin()));
  received_packet = packet_receiver_on_receiver.TakePacket();
  EXPECT_TRUE(received_packet);
  EXPECT_TRUE(
      std::equal(packet.begin(), packet.end(), (*received_packet).begin()));
}

// Test the sending/receiving as a UdpTransport.
TEST_F(UdpTransportImplTest, UdpTransportSendAndReceive) {
  std::string data = "Hello!";
  Packet packet(data.begin(), data.end());

  base::RunLoop run_loop;
  MockPacketReceiver packet_receiver_on_sender(run_loop.QuitClosure());
  MockPacketReceiver packet_receiver_on_receiver(
      base::BindRepeating(&SendPacket, recv_transport_.get(), packet));
  mojom::UdpTransportReceiverPtr receiver_ptr;
  packet_receiver_on_sender.BindToRequest(mojo::MakeRequest(&receiver_ptr));
  send_transport_->StartReceiving(std::move(receiver_ptr));
  recv_transport_->StartReceiving(
      packet_receiver_on_receiver.packet_receiver());

  mojo::DataPipe data_pipe(5);
  send_transport_->StartSending(std::move(data_pipe.consumer_handle));
  UdpPacketPipeWriter writer(std::move(data_pipe.producer_handle));
  base::MockCallback<base::OnceClosure> done_callback;
  EXPECT_CALL(done_callback, Run()).Times(1);
  writer.Write(new base::RefCountedData<Packet>(packet), done_callback.Get());
  run_loop.Run();
  std::unique_ptr<Packet> received_packet =
      packet_receiver_on_sender.TakePacket();
  EXPECT_TRUE(received_packet);
  EXPECT_TRUE(
      std::equal(packet.begin(), packet.end(), received_packet->begin()));
  received_packet = packet_receiver_on_receiver.TakePacket();
  EXPECT_TRUE(received_packet);
  EXPECT_TRUE(
      std::equal(packet.begin(), packet.end(), (*received_packet).begin()));
}

// Test the sending/receiving with a MojoUdpTransportClient. When
// sending a packet from the client, the packet is first send to the connected
// UdpTransportImpl through the mojo data pipe, and then send out to the remote
// receiver over network.
TEST_F(UdpTransportImplTest, WithMojoUdpTransportClient) {
  std::string data = "Hello!";
  Packet packet(data.begin(), data.end());
  base::RunLoop run_loop;

  // The remote receiver will send back any received packet to sender.
  MockPacketReceiver packet_receiver_on_receiver(
      base::BindRepeating(&SendPacket, recv_transport_.get(), packet));
  recv_transport_->StartReceiving(
      packet_receiver_on_receiver.packet_receiver());

  // Creates the UdpTransportClient and connect it to |send_transport_|.
  mojom::UdpTransportPtr transport_host_ptr;
  mojo::MakeStrongBinding(std::move(send_transport_),
                          mojo::MakeRequest(&transport_host_ptr));
  auto transport_client_on_sender =
      std::make_unique<MojoUdpTransportClient>(std::move(transport_host_ptr));
  // Exits run loop when |transport_client_on_sender| receives any packet.
  MockPacketReceiver packet_receiver_on_sender(run_loop.QuitClosure());
  transport_client_on_sender->StartReceiving(
      packet_receiver_on_sender.packet_receiver());

  // Sends |packet|.
  SendPacket(transport_client_on_sender.get(), packet);
  run_loop.Run();

  // Verifies that the |packet| is received on the remote receiver.
  std::unique_ptr<Packet> received_packet =
      packet_receiver_on_receiver.TakePacket();
  EXPECT_TRUE(received_packet);
  EXPECT_TRUE(
      std::equal(packet.begin(), packet.end(), (*received_packet).begin()));

  // The remote receiver is expect to have sent back the received packet to
  // sender. Verifies that it is received by |transport_client_on_sender|.
  received_packet = packet_receiver_on_sender.TakePacket();
  EXPECT_TRUE(received_packet);
  EXPECT_TRUE(
      std::equal(packet.begin(), packet.end(), received_packet->begin()));
}

}  // namespace cast
}  // namespace media
