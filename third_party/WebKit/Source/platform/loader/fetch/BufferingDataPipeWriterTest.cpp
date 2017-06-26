// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/loader/fetch/BufferingDataPipeWriter.h"

#include <memory>
#include <random>

#include "platform/testing/TestingPlatformSupport.h"
#include "platform/wtf/PtrUtil.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace blink {
namespace {

TEST(BufferingDataPipeWriterTest, WriteMany) {
  ScopedTestingPlatformSupport<TestingPlatformSupportWithMockScheduler>
      platform;
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
  std::string input;

  for (size_t i = 0; i < total; ++i)
    input += static_cast<char>(engine() % 26 + 'A');

  auto writer = WTF::MakeUnique<BufferingDataPipeWriter>(
      std::move(producer), platform->CurrentThread()->GetWebTaskRunner());

  for (size_t i = 0; i < total;) {
    size_t size = std::min(total - i, writing_chunk_size);
    ASSERT_TRUE(writer->Write(input.data() + i, size));

    i += size;
  }

  writer->Finish();

  std::string output;

  while (true) {
    constexpr auto kNone = MOJO_READ_DATA_FLAG_NONE;
    char buffer[reading_chunk_size] = {};
    uint32_t size = reading_chunk_size;
    result = mojo::ReadDataRaw(consumer.get(), buffer, &size, kNone);

    if (result == MOJO_RESULT_SHOULD_WAIT) {
      platform->RunUntilIdle();
      continue;
    }
    if (result == MOJO_RESULT_FAILED_PRECONDITION)
      break;

    ASSERT_EQ(MOJO_RESULT_OK, result);
    for (size_t i = 0; i < size; ++i) {
      output += buffer[i];
    }
  }
  EXPECT_EQ(output, input);
}

}  // namespace
}  // namespace blink
