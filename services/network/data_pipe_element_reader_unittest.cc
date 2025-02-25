// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/network/data_pipe_element_reader.h"

#include <stdint.h>

#include <limits>
#include <memory>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "base/strings/string_number_conversions.h"
#include "mojo/common/data_pipe_utils.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "mojo/public/cpp/system/data_pipe.h"
#include "net/base/completion_callback.h"
#include "net/base/io_buffer.h"
#include "net/base/test_completion_callback.h"
#include "services/network/public/interfaces/data_pipe_getter.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"

// Most tests of this class are at the URLLoader layer. These tests focus on
// things too difficult to cover with integration tests.

namespace network {

namespace {

class PassThroughDataPipeGetter : public mojom::DataPipeGetter {
 public:
  explicit PassThroughDataPipeGetter() : binding_(this) {}

  network::mojom::DataPipeGetterPtr GetDataPipeGetterPtr() {
    EXPECT_FALSE(binding_.is_bound());

    network::mojom::DataPipeGetterPtr data_pipe_getter_ptr;
    binding_.Bind(mojo::MakeRequest(&data_pipe_getter_ptr));
    return data_pipe_getter_ptr;
  }

  void WaitForRead(mojo::ScopedDataPipeProducerHandle* write_pipe,
                   ReadCallback* read_callback) {
    DCHECK(!run_loop_);

    if (!write_pipe_.is_valid()) {
      run_loop_ = std::make_unique<base::RunLoop>();
      run_loop_->Run();
      run_loop_.reset();
    }

    EXPECT_TRUE(write_pipe_.is_valid());
    EXPECT_TRUE(read_callback_);

    *write_pipe = std::move(write_pipe_);
    *read_callback = std::move(read_callback_);
  }

 private:
  // network::mojom::DataPipeGetter implementation:
  void Read(mojo::ScopedDataPipeProducerHandle pipe,
            ReadCallback callback) override {
    EXPECT_FALSE(write_pipe_.is_valid());
    EXPECT_FALSE(read_callback_);

    write_pipe_ = std::move(pipe);
    read_callback_ = std::move(callback);

    if (run_loop_)
      run_loop_->Quit();
  }

  void Clone(network::mojom::DataPipeGetterRequest request) override {
    NOTIMPLEMENTED();
  }

  std::unique_ptr<base::RunLoop> run_loop_;

  mojo::Binding<network::mojom::DataPipeGetter> binding_;
  mojo::ScopedDataPipeProducerHandle write_pipe_;
  ReadCallback read_callback_;

  DISALLOW_COPY_AND_ASSIGN(PassThroughDataPipeGetter);
};

class DataPipeElementReaderTest : public testing::Test {
 public:
  DataPipeElementReaderTest()
      : element_reader_(nullptr, data_pipe_getter_.GetDataPipeGetterPtr()) {}

 protected:
  base::MessageLoopForIO message_loop_;
  PassThroughDataPipeGetter data_pipe_getter_;
  DataPipeElementReader element_reader_;
};

// Test the case where a second Init() call occurs when there's a pending Init()
// call in progress. The first call should be dropped, in favor of the second
// one.
TEST_F(DataPipeElementReaderTest, InitInterruptsInit) {
  // Value deliberately outside of the range of an uint32_t, to catch any
  // accidental conversions to an int.
  const uint64_t kResponseBodySize = std::numeric_limits<uint32_t>::max();

  // The network stack calls Init.
  net::TestCompletionCallback first_init_callback;
  EXPECT_EQ(net::ERR_IO_PENDING,
            element_reader_.Init(first_init_callback.callback()));

  // Wait for DataPipeGetter::Read() to be called.
  mojo::ScopedDataPipeProducerHandle first_write_pipe;
  network::mojom::DataPipeGetter::ReadCallback first_read_pipe_callback;
  data_pipe_getter_.WaitForRead(&first_write_pipe, &first_read_pipe_callback);

  // The network stack calls Init again, interrupting the previous call.
  net::TestCompletionCallback second_init_callback;
  EXPECT_EQ(net::ERR_IO_PENDING,
            element_reader_.Init(second_init_callback.callback()));

  // Wait for DataPipeGetter::Read() to be called again.
  mojo::ScopedDataPipeProducerHandle second_write_pipe;
  network::mojom::DataPipeGetter::ReadCallback second_read_pipe_callback;
  data_pipe_getter_.WaitForRead(&second_write_pipe, &second_read_pipe_callback);

  // Sending data on the first read pipe should do nothing.
  std::move(first_read_pipe_callback)
      .Run(net::ERR_FAILED, kResponseBodySize - 1);
  // Run any pending tasks, to make sure nothing unexpected is queued.
  base::RunLoop().RunUntilIdle();
  EXPECT_FALSE(first_init_callback.have_result());
  EXPECT_FALSE(second_init_callback.have_result());

  // Sending data on the second pipe should result in the second init callback
  // being invoked.
  std::move(second_read_pipe_callback).Run(net::OK, kResponseBodySize);
  EXPECT_EQ(net::OK, second_init_callback.WaitForResult());
  EXPECT_FALSE(first_init_callback.have_result());

  EXPECT_EQ(kResponseBodySize, element_reader_.GetContentLength());
  EXPECT_EQ(kResponseBodySize, element_reader_.BytesRemaining());
  EXPECT_FALSE(element_reader_.IsInMemory());

  // Try to read from the body.
  scoped_refptr<net::IOBufferWithSize> io_buffer(new net::IOBufferWithSize(10));
  net::TestCompletionCallback read_callback;
  EXPECT_EQ(net::ERR_IO_PENDING,
            element_reader_.Read(io_buffer.get(), io_buffer->size(),
                                 read_callback.callback()));

  // Writes to the first write pipe should either fail, or succeed but be
  // ignored.
  mojo::common::BlockingCopyFromString("foo", first_write_pipe);
  base::RunLoop().RunUntilIdle();
  EXPECT_FALSE(read_callback.have_result());
}

// Test the case where a second Init() call occurs when there's a pending Read()
// call in progress. The old Read() should be dropped, in favor of the new
// Init().
TEST_F(DataPipeElementReaderTest, InitInterruptsRead) {
  // Value deliberately outside of the range of an uint32_t, to catch any
  // accidental conversions to an int.
  const uint64_t kResponseBodySize = std::numeric_limits<uint32_t>::max();

  // The network stack calls Init.
  net::TestCompletionCallback first_init_callback;
  EXPECT_EQ(net::ERR_IO_PENDING,
            element_reader_.Init(first_init_callback.callback()));

  // Wait for DataPipeGetter::Read() to be called.
  mojo::ScopedDataPipeProducerHandle first_write_pipe;
  network::mojom::DataPipeGetter::ReadCallback first_read_pipe_callback;
  data_pipe_getter_.WaitForRead(&first_write_pipe, &first_read_pipe_callback);
  std::move(first_read_pipe_callback).Run(net::OK, kResponseBodySize);

  ASSERT_EQ(net::OK, first_init_callback.WaitForResult());

  scoped_refptr<net::IOBufferWithSize> first_io_buffer(
      new net::IOBufferWithSize(10));
  net::TestCompletionCallback first_read_callback;
  EXPECT_EQ(net::ERR_IO_PENDING,
            element_reader_.Read(first_io_buffer.get(), first_io_buffer->size(),
                                 first_read_callback.callback()));

  // The network stack calls Init again, interrupting the previous read.
  net::TestCompletionCallback second_init_callback;
  EXPECT_EQ(net::ERR_IO_PENDING,
            element_reader_.Init(second_init_callback.callback()));

  // Wait for DataPipeGetter::Read() to be called again.
  mojo::ScopedDataPipeProducerHandle second_write_pipe;
  network::mojom::DataPipeGetter::ReadCallback second_read_pipe_callback;
  data_pipe_getter_.WaitForRead(&second_write_pipe, &second_read_pipe_callback);

  // Run any pending tasks, to make sure nothing unexpected is queued.
  base::RunLoop().RunUntilIdle();
  EXPECT_FALSE(first_read_callback.have_result());
  EXPECT_FALSE(second_init_callback.have_result());

  // Sending data on the second pipe should result in the second init callback
  // being invoked.
  std::move(second_read_pipe_callback).Run(net::OK, kResponseBodySize);
  EXPECT_EQ(net::OK, second_init_callback.WaitForResult());

  EXPECT_EQ(kResponseBodySize, element_reader_.GetContentLength());
  EXPECT_EQ(kResponseBodySize, element_reader_.BytesRemaining());
  EXPECT_FALSE(element_reader_.IsInMemory());

  // Try to read from the body.
  scoped_refptr<net::IOBufferWithSize> io_buffer(new net::IOBufferWithSize(10));
  net::TestCompletionCallback second_read_callback;
  EXPECT_EQ(net::ERR_IO_PENDING,
            element_reader_.Read(io_buffer.get(), io_buffer->size(),
                                 second_read_callback.callback()));

  // Writes to the first write pipe should either fail, or succeed but be
  // ignored.
  mojo::common::BlockingCopyFromString("foo", first_write_pipe);
  base::RunLoop().RunUntilIdle();
  EXPECT_FALSE(second_read_callback.have_result());
}

}  // namespace

}  // namespace network
