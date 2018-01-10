// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/network/public/cpp/buffering_data_pipe_writer.h"

namespace network {

BufferingDataPipeWriter::BufferingDataPipeWriter(
    mojo::ScopedDataPipeProducerHandle handle,
    scoped_refptr<base::SequencedTaskRunner> runner)
    : handle_(std::move(handle)),
      watcher_(FROM_HERE,
               mojo::SimpleWatcher::ArmingPolicy::MANUAL,
               std::move(runner)) {
  watcher_.Watch(handle_.get(), MOJO_HANDLE_SIGNAL_WRITABLE,
                 MOJO_WATCH_CONDITION_SATISFIED,
                 base::BindRepeating(&BufferingDataPipeWriter::OnWritable,
                                     base::Unretained(this)));
}

BufferingDataPipeWriter::~BufferingDataPipeWriter() {}

bool BufferingDataPipeWriter::Write(const char* buffer, uint32_t num_bytes) {
  DCHECK(!finished_);
  if (!handle_.is_valid())
    return false;

  if (buffer_.empty()) {
    while (num_bytes > 0) {
      uint32_t size = num_bytes;
      MojoResult result =
          handle_->WriteData(buffer, &size, MOJO_WRITE_DATA_FLAG_NONE);
      if (result == MOJO_RESULT_SHOULD_WAIT)
        break;
      if (result != MOJO_RESULT_OK) {
        Clear(result);
        return false;
      }
      num_bytes -= size;
      buffer += size;
    }
  }
  if (num_bytes == 0)
    return true;

  buffer_.emplace_back(buffer, buffer + num_bytes);
  if (!waiting_) {
    waiting_ = true;
    watcher_.ArmOrNotify();
  }
  return true;
}

bool BufferingDataPipeWriter::Finish(FinishCallback callback) {
  finished_ = true;

  if (buffer_.empty()) {
    Clear(MOJO_RESULT_OK);
    return true;
  }

  finish_callback_ = std::move(callback);
  return false;
}

void BufferingDataPipeWriter::OnWritable(MojoResult,
                                         const mojo::HandleSignalsState&) {
  if (!handle_.is_valid()) {
    Clear(MOJO_RESULT_UNKNOWN);
    return;
  }
  waiting_ = false;
  while (!buffer_.empty()) {
    std::vector<char>& front = buffer_.front();

    uint32_t size = front.size() - front_written_size_;

    MojoResult result = handle_->WriteData(front.data() + front_written_size_,
                                           &size, MOJO_WRITE_DATA_FLAG_NONE);
    if (result == MOJO_RESULT_SHOULD_WAIT) {
      waiting_ = true;
      watcher_.ArmOrNotify();
      return;
    }
    if (result != MOJO_RESULT_OK) {
      Clear(result);
      return;
    }
    front_written_size_ += size;

    if (front_written_size_ == front.size()) {
      front_written_size_ = 0;
      buffer_.pop_front();
    }
  }
  if (finished_ && buffer_.empty())
    Clear(MOJO_RESULT_OK);
}

void BufferingDataPipeWriter::Clear(MojoResult result) {
  handle_.reset();
  watcher_.Cancel();
  buffer_.clear();

  if (finish_callback_)
    std::move(finish_callback_).Run(result);
}

}  // namespace network
