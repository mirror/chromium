// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "storage/browser/blob/blob_transport_strategy.h"

#include "mojo/public/cpp/system/data_pipe.h"
#include "storage/browser/blob/blob_data_builder.h"
#include "storage/public/interfaces/blobs.mojom.h"

namespace storage {

namespace {

using MemoryStrategy = BlobMemoryController::Strategy;

// Transport strategy when no transport is needed. All Bytes elements should
// have their data embedded already.
class NoneNeededTransportStrategy : public BlobTransportStrategy {
 public:
  NoneNeededTransportStrategy(BlobDataBuilder* builder,
                              ResultCallback result_callback)
      : BlobTransportStrategy(builder, std::move(result_callback)) {}

  void AddBytesElement(mojom::DataElementBytes* bytes) override {
    DCHECK(bytes->embedded_data);
    DCHECK_EQ(bytes->length, bytes->embedded_data->size());
    builder_->AppendData(
        reinterpret_cast<const char*>(bytes->embedded_data->data()),
        bytes->length);
  }

  void BeginTransport(
      std::vector<BlobMemoryController::FileCreationInfo>) override {
    std::move(result_callback_).Run(BlobStatus::DONE);
  }
};

// Transport strategy that requests all data as replies.
class ReplyTransportStrategy : public BlobTransportStrategy {
 public:
  ReplyTransportStrategy(BlobDataBuilder* builder,
                         ResultCallback result_callback)
      : BlobTransportStrategy(builder, std::move(result_callback)) {}

  void AddBytesElement(mojom::DataElementBytes* bytes) override {
    size_t element_index = builder_->AppendFutureData(bytes->length);
    // base::Unretained is safe because |this| is guaranteed (by the contract
    // that code using BlobTransportStrategy should adhere to) to outlive the
    // BytesProvider.
    requests_.push_back(base::BindOnce(
        &mojom::BytesProvider::RequestAsReply,
        base::Unretained(bytes->data.get()),
        base::BindOnce(&ReplyTransportStrategy::OnReply, base::Unretained(this),
                       element_index, bytes->length)));
  }

  void BeginTransport(
      std::vector<BlobMemoryController::FileCreationInfo>) override {
    if (requests_.empty()) {
      std::move(result_callback_).Run(BlobStatus::DONE);
      return;
    }
    for (auto& request : requests_)
      std::move(request).Run();
  }

 private:
  void OnReply(size_t element_index,
               size_t expected_size,
               const std::vector<uint8_t>& data) {
    if (data.size() != expected_size) {
      mojo::ReportBadMessage(
          "Invalid data size in reply to BytesProvider::RequestAsReply");
      std::move(result_callback_)
          .Run(BlobStatus::ERR_INVALID_CONSTRUCTION_ARGUMENTS);
      return;
    }
    bool populate_result = builder_->PopulateFutureData(
        element_index, reinterpret_cast<const char*>(data.data()), 0,
        data.size());
    DCHECK(populate_result);

    if (++num_resolved_requests_ == requests_.size())
      std::move(result_callback_).Run(BlobStatus::DONE);
  }

  std::vector<base::OnceClosure> requests_;
  size_t num_resolved_requests_ = 0;
};

// Transport strategy that requests all data as data pipes, one pipe at a time.
class DataPipeTransportStrategy : public BlobTransportStrategy {
 public:
  DataPipeTransportStrategy(BlobDataBuilder* builder,
                            ResultCallback result_callback,
                            const BlobStorageLimits& limits)
      : BlobTransportStrategy(builder, std::move(result_callback)),
        limits_(limits),
        watcher_(FROM_HERE, mojo::SimpleWatcher::ArmingPolicy::AUTOMATIC) {}

  void AddBytesElement(mojom::DataElementBytes* bytes) override {
    // Split up the data in |max_bytes_data_item_size| sized chunks.
    for (size_t offset = 0; offset < bytes->length;
         offset += limits_.max_bytes_data_item_size) {
      size_t block_idx = builder_->AppendFutureData(std::min<size_t>(
          bytes->length - offset, limits_.max_bytes_data_item_size));
      if (offset == 0) {
        requests_.push_back(base::BindOnce(
            &DataPipeTransportStrategy::RequestDataPipe, base::Unretained(this),
            bytes->data.get(), bytes->length, block_idx));
      }
    }
  }

  void BeginTransport(
      std::vector<BlobMemoryController::FileCreationInfo>) override {
    NextRequestOrDone();
  }

 private:
  void NextRequestOrDone() {
    if (requests_.empty()) {
      std::move(result_callback_).Run(BlobStatus::DONE);
      return;
    }
    auto request = std::move(requests_.front());
    requests_.pop_front();
    std::move(request).Run();
  }

  void RequestDataPipe(mojom::BytesProvider* provider,
                       size_t expected_length,
                       size_t first_element_index) {
    DCHECK(!consumer_handle_.is_valid());
    mojo::ScopedDataPipeProducerHandle producer_handle;
    MojoCreateDataPipeOptions options;
    options.struct_size = sizeof(MojoCreateDataPipeOptions);
    options.flags = MOJO_CREATE_DATA_PIPE_OPTIONS_FLAG_NONE;
    options.element_num_bytes = 1;
    options.capacity_num_bytes = limits_.max_shared_memory_size;
    MojoResult result =
        CreateDataPipe(&options, &producer_handle, &consumer_handle_);
    if (result != MOJO_RESULT_OK) {
      DVLOG(1) << "Unable to create data pipe for blob transfer.";
      std::move(result_callback_).Run(BlobStatus::ERR_OUT_OF_MEMORY);
      return;
    }

    current_offset_ = 0;
    provider->RequestAsStream(std::move(producer_handle));
    watcher_.Watch(consumer_handle_.get(), MOJO_HANDLE_SIGNAL_READABLE,
                   base::Bind(&DataPipeTransportStrategy::OnDataPipeReadable,
                              base::Unretained(this), expected_length,
                              first_element_index));
  }

  void OnDataPipeReadable(size_t expected_length,
                          size_t first_element_index,
                          MojoResult result) {
    // The index of the element data should currently be written to, relative to
    // the first element of this stream (first_element_index).
    size_t relative_element_index =
        current_offset_ / limits_.max_bytes_data_item_size;
    // The offset into the current element where data should be written next.
    size_t offset_in_block =
        current_offset_ -
        relative_element_index * limits_.max_bytes_data_item_size;

    while (true) {
      uint32_t num_bytes = 0;
      const void* source_buffer;
      MojoResult read_result =
          mojo::BeginReadDataRaw(consumer_handle_.get(), &source_buffer,
                                 &num_bytes, MOJO_READ_DATA_FLAG_NONE);
      if (read_result == MOJO_RESULT_SHOULD_WAIT)
        break;
      if (read_result != MOJO_RESULT_OK) {
        // Data pipe broke before we received all the data.
        std::move(result_callback_).Run(BlobStatus::ERR_SOURCE_DIED_IN_TRANSIT);
        return;
      }

      if (num_bytes + current_offset_ > expected_length) {
        // Received more bytes then expected.
        std::move(result_callback_)
            .Run(BlobStatus::ERR_INVALID_CONSTRUCTION_ARGUMENTS);
        return;
      }

      // Only read as many bytes as can fit in current data element. Any
      // remaining bytes will be read on the next iteration of this loop.
      uint32_t bytes_to_read = std::min<uint32_t>(
          num_bytes, limits_.max_bytes_data_item_size - offset_in_block);
      char* output_buffer = builder_->GetFutureDataPointerToPopulate(
          first_element_index + relative_element_index, offset_in_block,
          bytes_to_read);
      DCHECK(output_buffer);

      std::memcpy(output_buffer, source_buffer, bytes_to_read);
      read_result = mojo::EndReadDataRaw(consumer_handle_.get(), bytes_to_read);
      DCHECK_EQ(read_result, MOJO_RESULT_OK);

      current_offset_ += bytes_to_read;
      if (current_offset_ >= expected_length) {
        // Done with this stream, on to the next.
        // TODO(mek): Should this wait to see if more data than expected gets
        // written, instead of immediately closing the pipe?
        watcher_.Cancel();
        consumer_handle_.reset();
        NextRequestOrDone();
        return;
      }

      offset_in_block += bytes_to_read;
      if (offset_in_block >= limits_.max_bytes_data_item_size) {
        offset_in_block = 0;
        relative_element_index++;
      }
    }
  }

  const BlobStorageLimits& limits_;
  std::deque<base::OnceClosure> requests_;

  mojo::ScopedDataPipeConsumerHandle consumer_handle_;
  mojo::SimpleWatcher watcher_;
  // How many bytes have been read and processed so far from the current data
  // pipe.
  size_t current_offset_ = 0;
};

// Transport strategy that requests all data through files.
class FileTransportStrategy : public BlobTransportStrategy {
 public:
  FileTransportStrategy(BlobDataBuilder* builder,
                        ResultCallback result_callback,
                        const BlobStorageLimits& limits)
      : BlobTransportStrategy(builder, std::move(result_callback)),
        limits_(limits) {}

  void AddBytesElement(mojom::DataElementBytes* bytes) override {
    uint64_t source_offset = 0;
    while (source_offset < bytes->length) {
      // Make sure no single file gets too big, but do use up all the available
      // space in all but the last file.
      uint64_t element_size =
          std::min(bytes->length - source_offset,
                   limits_.max_file_size - current_file_size_);
      size_t element_index = builder_->AppendFutureFile(
          current_file_size_, element_size, current_file_index_);

      requests_.push_back(Request{bytes->data.get(), source_offset,
                                  element_size, current_file_size_,
                                  current_file_index_, element_index});

      source_offset += element_size;
      current_file_size_ += element_size;
      if (current_file_size_ >= limits_.max_file_size) {
        current_file_size_ = 0;
        current_file_index_++;
      }
    }
  }

  void BeginTransport(
      std::vector<BlobMemoryController::FileCreationInfo> file_infos) override {
    if (requests_.empty()) {
      std::move(result_callback_).Run(BlobStatus::DONE);
      return;
    }
    DCHECK_EQ(file_infos.size(), requests_.back().file_index + 1);
    for (const auto& request : requests_) {
      // base::Unretained is safe because |this| is guaranteed (by the contract
      // that code using BlobTransportStrategy should adhere to) to outlive the
      // BytesProvider.
      request.provider->RequestAsFile(
          request.source_offset, request.source_size,
          file_infos[request.file_index].file.Duplicate(), request.file_offset,
          base::BindOnce(&FileTransportStrategy::OnReply,
                         base::Unretained(this), request.builder_element_index,
                         file_infos[request.file_index].file_reference));
    }
  }

 private:
  void OnReply(size_t builder_element_index,
               const scoped_refptr<ShareableFileReference>& file_reference,
               base::Optional<base::Time> time_file_modified) {
    bool populate_result =
        builder_->PopulateFutureFile(builder_element_index, file_reference,
                                     time_file_modified.value_or(base::Time()));
    DCHECK(populate_result);

    if (++num_resolved_requests_ == requests_.size())
      std::move(result_callback_).Run(BlobStatus::DONE);
  }

  const BlobStorageLimits& limits_;

  // State used to assign bytes elements to individual files.
  // The index of the first file that still has available space.
  size_t current_file_index_ = 0;
  // How big the current file already is.
  uint64_t current_file_size_ = 0;

  struct Request {
    // The BytesProvider to request this particular bit of data from.
    mojom::BytesProvider* provider;
    // Offset into the BytesProvider of the data to request.
    uint64_t source_offset;
    // Size of the bytes to request.
    uint64_t source_size;
    // Offset into the output file where data should be written.
    uint64_t file_offset;
    // Index of the file data should be written to.
    size_t file_index;
    // Index of the element in the BlobDataBuilder the data should be populated
    // into.
    size_t builder_element_index;
  };
  std::vector<Request> requests_;

  size_t num_resolved_requests_ = 0;
};

}  // namespace

// static
std::unique_ptr<BlobTransportStrategy> BlobTransportStrategy::Create(
    MemoryStrategy strategy,
    BlobDataBuilder* builder,
    ResultCallback result_callback,
    const BlobStorageLimits& limits) {
  switch (strategy) {
    case MemoryStrategy::NONE_NEEDED:
      return base::MakeUnique<NoneNeededTransportStrategy>(
          builder, std::move(result_callback));
    case MemoryStrategy::IPC:
      return base::MakeUnique<ReplyTransportStrategy>(
          builder, std::move(result_callback));
    case MemoryStrategy::SHARED_MEMORY:
      return base::MakeUnique<DataPipeTransportStrategy>(
          builder, std::move(result_callback), limits);
    case MemoryStrategy::FILE:
      return base::MakeUnique<FileTransportStrategy>(
          builder, std::move(result_callback), limits);
    case MemoryStrategy::TOO_LARGE:
      NOTREACHED();
  }
  NOTREACHED();
  return nullptr;
}

BlobTransportStrategy::~BlobTransportStrategy() {}

BlobTransportStrategy::BlobTransportStrategy(BlobDataBuilder* builder,
                                             ResultCallback result_callback)
    : builder_(builder), result_callback_(std::move(result_callback)) {}

}  // namespace storage
