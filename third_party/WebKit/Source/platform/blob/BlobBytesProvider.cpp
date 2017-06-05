// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/blob/BlobBytesProvider.h"

#include "platform/wtf/Functional.h"
#include "public/platform/Platform.h"

namespace blink {

namespace {

class BlobBytesStreamer {
 public:
  BlobBytesStreamer(Vector<RefPtr<RawData>> data,
                    mojo::ScopedDataPipeProducerHandle pipe)
      : data_(std::move(data)),
        pipe_(std::move(pipe)),
        watcher_(FROM_HERE, mojo::SimpleWatcher::ArmingPolicy::AUTOMATIC) {
    watcher_.Watch(pipe_.get(), MOJO_HANDLE_SIGNAL_WRITABLE,
                   ConvertToBaseCallback(WTF::Bind(
                       &BlobBytesStreamer::OnWritable, WTF::Unretained(this))));
  }

  void OnWritable(MojoResult result) {
    if (result == MOJO_RESULT_CANCELLED ||
        result == MOJO_RESULT_FAILED_PRECONDITION) {
      delete this;
      return;
    }
    DCHECK_EQ(result, MOJO_RESULT_OK);

    while (true) {
      uint32_t num_bytes =
          data_[current_item_]->length() - current_item_offset_;
      MojoResult write_result = mojo::WriteDataRaw(
          pipe_.get(), data_[current_item_]->data() + current_item_offset_,
          &num_bytes, MOJO_WRITE_DATA_FLAG_NONE);
      if (write_result == MOJO_RESULT_OK) {
        current_item_offset_ += num_bytes;
        if (current_item_offset_ >= data_[current_item_]->length()) {
          current_item_++;
          current_item_offset_ = 0;
          if (current_item_ >= int(data_.size())) {
            // Sent all items completely, done.
            delete this;
            return;
          }
        }
      } else if (write_result == MOJO_RESULT_SHOULD_WAIT) {
        break;
      } else {
        // Something went wrong.
        delete this;
        return;
      }
    }
  }

 private:
  int current_item_ = 0;
  size_t current_item_offset_ = 0;
  Vector<RefPtr<RawData>> data_;

  mojo::ScopedDataPipeProducerHandle pipe_;
  mojo::SimpleWatcher watcher_;
};

}  // namespace

BlobBytesProvider::BlobBytesProvider(RefPtr<RawData> data) {
  // TODO(mek): This is probably not enough to keep the renderer alive while
  // data is being transferred.
  Platform::Current()->SuddenTerminationChanged(false);
  data_.push_back(std::move(data));
}

BlobBytesProvider::~BlobBytesProvider() {
  Platform::Current()->SuddenTerminationChanged(true);
}

void BlobBytesProvider::AppendData(RefPtr<RawData> data) {
  data_.push_back(std::move(data));
}

void BlobBytesProvider::RequestAsReply(RequestAsReplyCallback callback) {
  Vector<uint8_t> result;
  for (const auto& d : data_)
    result.Append(d->data(), d->length());
  std::move(callback).Run(result);
}

void BlobBytesProvider::RequestAsStream(
    mojo::ScopedDataPipeProducerHandle pipe) {
  // Will self delete when done.
  new BlobBytesStreamer(data_, std::move(pipe));
}

void BlobBytesProvider::RequestAsFile(uint64_t source_offset,
                                      uint64_t source_size,
                                      base::File file,
                                      uint64_t file_offset,
                                      RequestAsFileCallback callback) {
  // TODO(mek): Do on correct thread
  if (!file.IsValid()) {
    LOG(INFO) << "Not valid";
    std::move(callback).Run(WTF::nullopt);
    return;
  }

  int64_t seek_distance =
      file.Seek(base::File::FROM_BEGIN, SafeCast<int64_t>(file_offset));
  bool seek_failed = seek_distance < 0;
  // histogram Storage.Blob.RendererFileSeekFailed
  if (seek_failed) {
    LOG(INFO) << "Seek failed";
    std::move(callback).Run(WTF::nullopt);
    return;
  }

  // TODO(mek): More efficient way to find beginning.
  // Offset of the current data chunk in the overall stream provided by this
  // provider.
  uint64_t offset = 0;
  for (const RefPtr<RawData>& data : data_) {
    // Skip any chunks that are entirely before the data we need to write.
    if (offset + data->length() <= source_offset) {
      offset += data->length();
      continue;
    }

    // We're done if the beginning of the current chunk is past the end of the
    // data to write.
    if (offset >= source_offset + source_size)
      break;

    // Offset within this chunk where writing needs to start from.
    uint64_t data_offset = offset > source_offset ? 0 : source_offset - offset;
    uint64_t data_size =
        std::min(data->length() - data_offset,
                 source_offset + source_size - offset - data_offset);
    size_t written = 0;
    while (written < data_size) {
      size_t writing_size = data_size - written;
      int actual_written = file.WriteAtCurrentPos(
          data->data() + data_offset + written, static_cast<int>(writing_size));
      bool write_failed = actual_written < 0;
      // histogram Storage.Blob.RendererFileWriteFailed
      if (write_failed) {
        LOG(INFO) << "Write failed";
        std::move(callback).Run(WTF::nullopt);
        return;
      }
      written += actual_written;
    }

    offset += data->length();
  }

  if (!file.Flush()) {
    LOG(INFO) << "Flush failed";
    std::move(callback).Run(WTF::nullopt);
    return;
  }
  base::File::Info info;
  if (!file.GetInfo(&info)) {
    LOG(INFO) << "GetInfo failed";
    std::move(callback).Run(WTF::nullopt);
    return;
  }
  std::move(callback).Run(info.last_modified);
}

}  // namespace blink
