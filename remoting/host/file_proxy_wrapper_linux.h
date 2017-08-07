// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_HOST_FILE_PROXY_WRAPPER_LINUX_H_
#define REMOTING_HOST_FILE_PROXY_WRAPPER_LINUX_H_

#include "remoting/host/file_proxy_wrapper.h"

namespace base {
class SequencedTaskRunner;
}  // namespace base

namespace remoting {

class FileProxyWrapperLinux : public FileProxyWrapper {
 public:
  FileProxyWrapperLinux();
  ~FileProxyWrapperLinux() override;

  // FileProxyWrapper implementation.
  void Init(const ErrorCallback& error_callback) override;
  void CreateFile(const std::string& filename,
                  uint64_t filesize,
                  const SuccessCallback& success_callback) override;
  void WriteChunk(std::unique_ptr<CompoundBuffer> buffer) override;
  void Close(const SuccessCallback& success_callback) override;
  void Cancel() override;

 private:
  scoped_refptr<base::SequencedTaskRunner> file_task_runner_;
};

}  // namespace remoting

#endif  // REMOTING_HOST_FILE_PROXY_WRAPPER_LINUX_H_
