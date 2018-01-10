// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/network/public/cpp/buffering_data_pipe_writer.h"

#include <memory>
#include <random>

#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace network {
namespace {

class BufferingDataPipeWriterTest : public ::testing::Test {
 protected:
  static void OnFinished(base::OnceClosure quit_closure,
                         MojoResult expected_result,
                         MojoResult actual_result) {
    EXPECT_EQ(expected_result, actual_result);
    std::move(quit_closure).Run();
  }

  base::MessageLoop message_loop_;
};

TEST_F(BufferingDataPipeWriterTest, WriteMany) {
  constexpr int kCapacity = 4096;

  std::minstd_rand engine(99);

  mojo::ScopedDataPipeProducerHandle producer;
  mojo::ScopedDataPipeConsumerHandle consumer;
  MojoCreateDataPipeOptions options;
  options.struct_size = sizeof(MojoCreateDataPipeOptions);
  options.flags = MOJO_CREATE_DATA_PIPE_OPTIONS_FLAG_NONE;
  options.element_num_bytes = 1;
  options.capacity_num_bytes = kCapacity;

  MojoResult result = mojo::CreateDataPipe(&options, &producer, &consumer);
  ASSERT_EQ(MOJO_RESULT_OK, result);

  constexpr size_t total = kCapacity * 3;
  constexpr size_t writing_chunk_size = 5;
  constexpr size_t reading_chunk_size = 7;
  std::vector<char> input, output;

  for (size_t i = 0; i < total; ++i)
    input.push_back(static_cast<char>(engine() % 26 + 'A'));

  auto writer = std::make_unique<BufferingDataPipeWriter>(
      std::move(producer), base::SequencedTaskRunnerHandle::Get());

  for (size_t i = 0; i < total;) {
    // We use a temporary buffer to check that the buffer is copied immediately.
    char temp[writing_chunk_size] = {};
    size_t size = std::min(total - i, writing_chunk_size);

    std::copy(input.data() + i, input.data() + i + size, temp);
    ASSERT_TRUE(writer->Write(temp, size));

    i += size;
  }

  base::RunLoop run_loop;
  EXPECT_FALSE(writer->Finish(
      base::BindOnce(OnFinished, run_loop.QuitClosure(), MOJO_RESULT_OK)));

  while (true) {
    constexpr auto kNone = MOJO_READ_DATA_FLAG_NONE;
    char buffer[reading_chunk_size] = {};
    uint32_t size = reading_chunk_size;
    result = consumer->ReadData(buffer, &size, kNone);

    if (result == MOJO_RESULT_SHOULD_WAIT) {
      base::RunLoop().RunUntilIdle();
      continue;
    }
    if (result == MOJO_RESULT_FAILED_PRECONDITION)
      break;

    ASSERT_EQ(MOJO_RESULT_OK, result);
    output.insert(output.end(), buffer, buffer + size);
  }
  EXPECT_EQ(output, input);
  run_loop.Run();
}

TEST_F(BufferingDataPipeWriterTest, FailedPrecondition) {
  constexpr int kCapacity = 4096;

  std::minstd_rand engine(99);

  mojo::ScopedDataPipeProducerHandle producer;
  mojo::ScopedDataPipeConsumerHandle consumer;
  MojoCreateDataPipeOptions options;
  options.struct_size = sizeof(MojoCreateDataPipeOptions);
  options.flags = MOJO_CREATE_DATA_PIPE_OPTIONS_FLAG_NONE;
  options.element_num_bytes = 1;
  options.capacity_num_bytes = kCapacity;

  MojoResult result = mojo::CreateDataPipe(&options, &producer, &consumer);
  ASSERT_EQ(MOJO_RESULT_OK, result);

  constexpr size_t total = kCapacity * 3;
  constexpr size_t writing_chunk_size = 5;
  std::vector<char> input;

  for (size_t i = 0; i < total; ++i)
    input.push_back(static_cast<char>(engine() % 26 + 'A'));

  auto writer = std::make_unique<BufferingDataPipeWriter>(
      std::move(producer), base::SequencedTaskRunnerHandle::Get());

  for (size_t i = 0; i < total;) {
    // We use a temporary buffer to check that the buffer is copied immediately.
    char temp[writing_chunk_size] = {};
    size_t size = std::min(total - i, writing_chunk_size);

    std::copy(input.data() + i, input.data() + i + size, temp);
    ASSERT_TRUE(writer->Write(temp, size));

    i += size;
  }

  base::RunLoop run_loop;
  EXPECT_FALSE(writer->Finish(base::BindOnce(OnFinished, run_loop.QuitClosure(),
                                             MOJO_RESULT_FAILED_PRECONDITION)));

  consumer.reset();
  run_loop.Run();
}

TEST_F(BufferingDataPipeWriterTest, FinishSynchronously) {
  constexpr int kCapacity = 4096;

  std::minstd_rand engine(99);

  mojo::ScopedDataPipeProducerHandle producer;
  mojo::ScopedDataPipeConsumerHandle consumer;
  MojoCreateDataPipeOptions options;
  options.struct_size = sizeof(MojoCreateDataPipeOptions);
  options.flags = MOJO_CREATE_DATA_PIPE_OPTIONS_FLAG_NONE;
  options.element_num_bytes = 1;
  options.capacity_num_bytes = kCapacity;

  MojoResult result = mojo::CreateDataPipe(&options, &producer, &consumer);
  ASSERT_EQ(MOJO_RESULT_OK, result);

  auto writer = std::make_unique<BufferingDataPipeWriter>(
      std::move(producer), base::SequencedTaskRunnerHandle::Get());

  base::RunLoop run_loop;
  MojoResult unexpected_result = MOJO_RESULT_BUSY;
  EXPECT_TRUE(writer->Finish(
      base::BindOnce(OnFinished, run_loop.QuitClosure(), unexpected_result)));

  // OnFinished should not be called.
  run_loop.RunUntilIdle();
}

}  // namespace
}  // namespace network
