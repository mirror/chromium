// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_OFFLINE_PAGES_CORE_PREFETCH_STORE_STORE_COMMAND_H_
#define COMPONENTS_OFFLINE_PAGES_CORE_PREFETCH_STORE_STORE_COMMAND_H_

#include "base/macros.h"
#include "base/memory/ref_counted.h"

namespace base {
class SingleThreadTaskRunner;
}

namespace sql {
class Connection;
}

namespace offline_pages {
class StoreCommand {
 public:
  StoreCommand() = default;
  virtual ~StoreCommand() = default;
  virtual void Execute(
      sql::Connection* db,
      scoped_refptr<base::SingleThreadTaskRunner> background_runner) = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(StoreCommand);
};
}  // namespace offline_pages

#endif  // COMPONENTS_OFFLINE_PAGES_CORE_PREFETCH_STORE_STORE_COMMAND_H_
