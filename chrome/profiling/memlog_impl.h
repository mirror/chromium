// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_PROFILING_MEMLOG_IMPL_H_
#define CHROME_PROFILING_MEMLOG_IMPL_H_

#include "base/macros.h"
#include "chrome/common/profiling/memlog.mojom.h"
#include "chrome/profiling/backtrace_storage.h"
#include "chrome/profiling/memlog_connection_manager.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "services/service_manager/public/cpp/service_context_ref.h"

namespace profiling {

// Represents the profiling process side of the profiling <-> browser
// connection. This class is not thread safe and must onle be called on the IO
// thread (which is the main thread in the profiling process).
class MemlogImpl : public mojom::Memlog {
 public:
  explicit MemlogImpl(
      std::unique_ptr<service_manager::ServiceContextRef> service_ref);
  ~MemlogImpl() override;

  void GetMemlogPipe(int32_t pid, GetMemlogPipeCallback callback) override;
  void DumpProcess(int32_t pid) override;

 private:
  std::unique_ptr<service_manager::ServiceContextRef> service_ref_;

  void AddPipeOnIO(base::ScopedPlatformFile file, int32_t pid);

  scoped_refptr<base::SequencedTaskRunner> io_runner_;
  BacktraceStorage backtrace_storage_;
  MemlogConnectionManager connection_manager_;

  DISALLOW_COPY_AND_ASSIGN(MemlogImpl);
};

}  // namespace profiling

#endif  // CHROME_PROFILING_MEMLOG_IMPL_H_
