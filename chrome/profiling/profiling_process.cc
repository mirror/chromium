// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/profiling/profiling_process.h"

#include "base/bind.h"
#include "chrome/profiling/profiling_globals.h"

namespace profiling {

ProfilingProcess::ProfilingProcess(mojom::ProfilingControlRequest request)
    : binding_(this) {
  ProfilingGlobals::Get()->GetIORunner()->PostTask(
      FROM_HERE, base::BindOnce(&ProfilingProcess::BindOnIO,
                                base::Unretained(this), std::move(request)));
}

ProfilingProcess::~ProfilingProcess() {}

void ProfilingProcess::AddNewSender(//mojo::ScopedHandle sender_pipe,
                                    int32_t sender_pid) {}

void ProfilingProcess::BindOnIO(mojom::ProfilingControlRequest request) {
  binding_.Bind(std::move(request));
}

}  // namespace profiling
