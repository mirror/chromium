// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_PROFILING_PROFILING_PROCESS_H_
#define CHROME_PROFILING_PROFILING_PROCESS_H_

#include "base/macros.h"
#include "chrome/common/profiling/profiling_control.mojom.h"
#include "mojo/public/cpp/bindings/binding.h"

namespace profiling {

class ProfilingProcess : public mojom::ProfilingControl {
 public:
  ProfilingProcess(mojom::ProfilingControlRequest request);
  ~ProfilingProcess();

  // ProfilingControl implementation.
  void AddNewSender(//mojo::ScopedHandle sender_pipe,
                    int32_t sender_pid) override;

 private:
  void BindOnIO(mojom::ProfilingControlRequest request);

  mojo::Binding<mojom::ProfilingControl> binding_;

  DISALLOW_COPY_AND_ASSIGN(ProfilingProcess);
};

}  // namespace profiling

#endif  // CHROME_PROFILING_PROFILING_PROCESS_H_
