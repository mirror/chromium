// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_CHILD_CHILD_PROCESS_CRASHER_H_
#define CONTENT_CHILD_CHILD_PROCESS_CRASHER_H_

#include "base/memory/ref_counted.h"
#include "content/common/child_process_crasher.mojom.h"
#include "content/common/content_export.h"

namespace base {
class SingleThreadTaskRunner;
}

namespace service_manager {
struct BindSourceInfo;
}

namespace content {

class CONTENT_EXPORT ChildProcessCrasherImpl
    : public mojom::ChildProcessCrasher {
 public:
  explicit ChildProcessCrasherImpl(
      scoped_refptr<base::SingleThreadTaskRunner> io_thread);
  ~ChildProcessCrasherImpl() override;

  static void Create(scoped_refptr<base::SingleThreadTaskRunner> io_thread,
                     const service_manager::BindSourceInfo&,
                     mojom::ChildProcessCrasherRequest request);
  void Crash(mojom::CrashKeysPtr crash_keys) override;

 private:
  scoped_refptr<base::SingleThreadTaskRunner> io_thread_;

  DISALLOW_COPY_AND_ASSIGN(ChildProcessCrasherImpl);
};

}  // namespace content

#endif  // CONTENT_CHILD_CHILD_PROCESS_CRASHER_H_
