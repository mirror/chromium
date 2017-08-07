// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/host/file_proxy_wrapper_linux.h"

#include "base/sequenced_task_runner.h"
#include "base/task_scheduler/post_task.h"
#include "remoting/base/compound_buffer.h"

namespace remoting {

FileProxyWrapperLinux::FileProxyWrapperLinux()
    : file_task_runner_(base::CreateSequencedTaskRunnerWithTraits(
          {base::MayBlock(), base::TaskPriority::BACKGROUND})) {
  // DCHECK(file_task_runner_);
}

FileProxyWrapperLinux::~FileProxyWrapperLinux() = default;

void FileProxyWrapperLinux::Init(const ErrorCallback& error_callback) {
  // TODO(jarhar): Implement Init.
}

void FileProxyWrapperLinux::CreateFile(
    const std::string& filename,
    uint64_t filesize,
    const SuccessCallback& success_callback) {
  // TODO(jarhar): Implement CreateFile.
}

void FileProxyWrapperLinux::WriteChunk(std::unique_ptr<CompoundBuffer> buffer) {
  // TODO(jarhar): Implement WriteChunk.
}

void FileProxyWrapperLinux::Close(const SuccessCallback& success_callback) {
  // TODO(jarhar): Implement Close.
}

void FileProxyWrapperLinux::Cancel() {
  // TODO(jarhar): Implement Cancel.
}

}  // namespace remoting
