// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_NETWORK_PUBLIC_CPP_BUFFERING_DATA_PIPE_WRITER_H_
#define SERVICES_NETWORK_PUBLIC_CPP_BUFFERING_DATA_PIPE_WRITER_H_

#include <deque>
#include <vector>

#include "base/callback.h"
#include "base/memory/ref_counted.h"
#include "base/sequenced_task_runner.h"
#include "mojo/public/cpp/system/data_pipe.h"
#include "mojo/public/cpp/system/simple_watcher.h"

namespace network {

// A writer to a mojo data pipe which has a buffer to store contents.
class BufferingDataPipeWriter final {
 public:
  using FinishCallback = base::OnceCallback<void(MojoResult)>;

  BufferingDataPipeWriter(mojo::ScopedDataPipeProducerHandle,
                          scoped_refptr<base::SequencedTaskRunner> runner);
  ~BufferingDataPipeWriter();

  // Writes buffer[0:num_bytes] to the data pipe. Returns true if there is no
  // error. Otherwise, users must not call Writer or Finish any more.
  bool Write(const char* buffer, uint32_t num_bytes);

  // Finishes writing. After calling this function, Write and Finish must not
  // be called.
  // Returns true if the internal buffer is already empty. Otherwise, after
  // writing all chunks or getting an error, |callback| will be called with the
  // result.
  bool Finish(FinishCallback callback);

 private:
  void OnWritable(MojoResult, const mojo::HandleSignalsState&);
  void ClearIfNeeded();
  void Clear(MojoResult result);

  mojo::ScopedDataPipeProducerHandle handle_;
  mojo::SimpleWatcher watcher_;
  std::deque<std::vector<char>> buffer_;
  FinishCallback finish_callback_;
  size_t front_written_size_ = 0;
  bool waiting_ = false;
  bool finished_ = false;
};

}  // namespace network

#endif  // SERVICES_NETWORK_PUBLIC_CPP_BUFFERING_DATA_PIPE_WRITER_H_
