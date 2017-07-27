// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/macros.h"
#include "chrome/common/profiling/profiling_control.mojom.h"
#include "services/service_manager/public/cpp/service.h"
#include "services/service_manager/public/cpp/service_context_ref.h"

namespace profiling {

class ProfilingControlImpl : public mojom::ProfilingControl {
 public:
  explicit ProfilingControlImpl(std::unique_ptr<service_manager::ServiceContextRef> service_ref);
  ~ProfilingControlImpl() override;

  static std::unique_ptr<service_manager::Service> CreateService();

  // ProfilingControl implementation.
  void AddNewSender(mojo::ScopedHandle sender_pipe,
                    int32_t sender_pid) override;

  void DumpProcess(int32_t pid) override;

 private:
  std::unique_ptr<service_manager::ServiceContextRef> service_ref_;

  DISALLOW_COPY_AND_ASSIGN(ProfilingControlImpl);
};

}  // namespace profiling

