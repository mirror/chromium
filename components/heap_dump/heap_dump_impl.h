// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_HEAP_DUMP_HEAP_DUMP_IMPL_H_
#define COMPONENTS_HEAP_DUMP_HEAP_DUMP_IMPL_H_

#include "components/heap_dump/heap_dump.mojom.h"

namespace heap_dump {

class HeapDumpImpl : public mojom::HeapDump {
 public:
  HeapDumpImpl();
  ~HeapDumpImpl() override;

  static void RegisterInterface();

 private:
  void RequestHeapDump(mojo::ScopedHandle file_handle,
                       RequestHeapDumpCallback callback) override;
};

}  // namespace heap_dump

#endif  // COMPONENTS_HEAP_DUMP_HEAP_DUMP_IMPL_H_
