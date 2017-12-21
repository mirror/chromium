// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/cast/net/udp_packet_pipe.h"

#include <deque>
#include <string>
#include <vector>

#include "base/bind.h"
#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "base/test/mock_callback.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace media {
namespace cast {

namespace {
constexpr uint32_t kDefaultDataPipeCapacityBytes = 10;
}  // namespace

class UdpPacketPipeTest : public ::testing::Test {
 public:
  UdpPacketPipeTest() {
    mojo::DataPipe data_pipe(kDefaultDataPipeCapacityBytes);
    writer_ = base::MakeUnique<UdpPacketPipeWriter>(
        std::move(data_pipe.producer_handle));
    reader_ = base::MakeUnique<UdpPacketPipeReader>(
        std::move(data_pipe.consumer_handle));
  }

  ~UdpPacketPipeTest() override = default;

  static void RunUntilIdle() { base::RunLoop().RunUntilIdle(); }

  void OnPacketRead(std::unique_ptr<Packet> packet) {
    packets_read_.push_back(std::move(packet));
  }

  base::MessageLoop message_loop_;

 protected:
  std::unique_ptr<UdpPacketPipeWriter> writer_;
  std::unique_ptr<UdpPacketPipeReader> reader_;
  std::deque<std::unique_ptr<Packet>> packets_read_;

 private:
  DISALLOW_COPY_AND_ASSIGN(UdpPacketPipeTest);
};

TEST_F(UdpPacketPipeTest, Normal) {
  std::string data1 = "test";
  std::string data2 = "Hello!";
  Packet packet1(data1.begin(), data1.end());
  Packet packet2(data2.begin(), data2.end());
  base::MockCallback<base::OnceClosure> done_callback;
  EXPECT_CALL(done_callback, Run()).Times(1);
  writer_->Write(new base::RefCountedData<Packet>(packet1),
                 done_callback.Get());
  RunUntilIdle();
  // Packet2 can not be completely written in the data pipe due to capacity
  // limit.
  EXPECT_CALL(done_callback, Run()).Times(0);
  writer_->Write(new base::RefCountedData<Packet>(packet2),
                 done_callback.Get());
  RunUntilIdle();

  EXPECT_TRUE(packets_read_.empty());
  // |packet2| is expected to complete writing after |packet1| was read.
  EXPECT_CALL(done_callback, Run()).Times(1);
  reader_->Read(
      base::BindOnce(&UdpPacketPipeTest::OnPacketRead, base::Unretained(this)));
  RunUntilIdle();
  EXPECT_EQ(1u, packets_read_.size());
  EXPECT_EQ(0, std::memcmp(packet1.data(), packets_read_.front()->data(),
                           packet1.size()));
  packets_read_.pop_front();
  reader_->Read(
      base::BindOnce(&UdpPacketPipeTest::OnPacketRead, base::Unretained(this)));
  RunUntilIdle();
  EXPECT_EQ(1u, packets_read_.size());
  EXPECT_EQ(0, std::memcmp(packet2.data(), packets_read_.front()->data(),
                           packet2.size()));
  packets_read_.pop_front();
  EXPECT_TRUE(packets_read_.empty());
}

}  // namespace cast
}  // namespace media
