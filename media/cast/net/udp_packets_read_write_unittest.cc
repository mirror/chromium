// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/cast/net/udp_packets_read_write.h"

#include <deque>
#include <string>
#include <vector>

#include "base/bind.h"
#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace media {
namespace cast {

namespace {
constexpr uint32_t kDefaultDataPipeCapacityBytes = 10;
}  // namespace

class UdpPacketsReadWriteTest : public ::testing::Test {
 public:
  UdpPacketsReadWriteTest() {
    mojo::DataPipe data_pipe(kDefaultDataPipeCapacityBytes);

    writer_ = base::MakeUnique<UdpPacketsWriter>(
        std::move(data_pipe.producer_handle));
    reader_ = base::MakeUnique<UdpPacketsReader>(
        std::move(data_pipe.consumer_handle),
        base::BindRepeating(&UdpPacketsReadWriteTest::OnPacketRead,
                            base::Unretained(this)));
  }

  ~UdpPacketsReadWriteTest() override = default;

  static void RunUntilIdle() { base::RunLoop().RunUntilIdle(); }

  base::MessageLoop message_loop_;

 protected:
  std::unique_ptr<UdpPacketsWriter> writer_;
  std::unique_ptr<UdpPacketsReader> reader_;
  std::deque<std::unique_ptr<Packet>> packets_read_;

 private:
  void OnPacketRead(std::unique_ptr<Packet> packet) {
    packets_read_.push_back(std::move(packet));
  }

  DISALLOW_COPY_AND_ASSIGN(UdpPacketsReadWriteTest);
};

TEST_F(UdpPacketsReadWriteTest, Normal) {
  std::string data1 = "test";
  std::string data2 = "Hello!";
  Packet packet1(data1.begin(), data1.end());
  Packet packet2(data2.begin(), data2.end());
  EXPECT_TRUE(packets_read_.empty());
  writer_->Write(new base::RefCountedData<Packet>(packet1));
  writer_->Write(new base::RefCountedData<Packet>(packet2));
  RunUntilIdle();
  EXPECT_EQ(2u, packets_read_.size());
  EXPECT_EQ(0, std::memcmp(packet1.data(), packets_read_.front()->data(),
                           packet1.size()));
  packets_read_.pop_front();
  EXPECT_EQ(0, std::memcmp(packet2.data(), packets_read_.front()->data(),
                           packet2.size()));
  packets_read_.pop_front();
  EXPECT_TRUE(packets_read_.empty());
}

}  // namespace cast
}  // namespace media
