// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "storage/browser/test/mock_bytes_provider.h"

#include "base/task_scheduler/post_task.h"
#include "base/threading/thread_restrictions.h"
#include "mojo/common/data_pipe_utils.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace storage {

namespace {

void BindBytesProvider(std::unique_ptr<MockBytesProvider> impl,
                       mojom::BytesProviderRequest request) {
  mojo::MakeStrongBinding(std::move(impl), std::move(request));
}

}  // namespace

// static
mojom::BytesProviderPtr MockBytesProvider::Create(std::vector<uint8_t> data) {
  mojom::BytesProviderPtr result;
  auto provider = base::MakeUnique<MockBytesProvider>(std::move(data));
  base::CreateSequencedTaskRunnerWithTraits(
      {base::MayBlock(), base::WithBaseSyncPrimitives()})
      ->PostTask(FROM_HERE,
                 base::BindOnce(&BindBytesProvider, std::move(provider),
                                MakeRequest(&result)));
  return result;
}

// static
mojom::BytesProviderPtr MockBytesProvider::Create(const std::string& data) {
  return Create(std::vector<uint8_t>(data.begin(), data.end()));
}

MockBytesProvider::MockBytesProvider(
    std::vector<uint8_t> data,
    size_t* reply_request_count,
    size_t* stream_request_count,
    size_t* file_request_count,
    base::Optional<base::Time> file_modification_time)
    : data_(std::move(data)),
      reply_request_count_(reply_request_count),
      stream_request_count_(stream_request_count),
      file_request_count_(file_request_count),
      file_modification_time_(file_modification_time) {}

MockBytesProvider::~MockBytesProvider() {}

void MockBytesProvider::RequestAsReply(RequestAsReplyCallback callback) {
  if (reply_request_count_)
    ++*reply_request_count_;
  std::move(callback).Run(data_);
}

void MockBytesProvider::RequestAsStream(
    mojo::ScopedDataPipeProducerHandle pipe) {
  if (stream_request_count_)
    ++*stream_request_count_;
  base::ScopedAllowBaseSyncPrimitivesForTesting allow_base_sync_primitives;
  mojo::common::BlockingCopyFromString(
      std::string(reinterpret_cast<const char*>(data_.data()), data_.size()),
      pipe);
}

void MockBytesProvider::RequestAsFile(uint64_t source_offset,
                                      uint64_t source_size,
                                      base::File file,
                                      uint64_t file_offset,
                                      RequestAsFileCallback callback) {
  if (file_request_count_)
    ++*file_request_count_;
  EXPECT_LE(source_offset + source_size, data_.size());
  EXPECT_EQ(source_size,
            static_cast<uint64_t>(file.Write(
                file_offset,
                reinterpret_cast<const char*>(data_.data() + source_offset),
                source_size)));
  EXPECT_TRUE(file.Flush());
  std::move(callback).Run(file_modification_time_);
}

}  // namespace storage
