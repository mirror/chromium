// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_DOWNLOAD_INTERNAL_LOGGER_IMPL_H_
#define COMPONENTS_DOWNLOAD_INTERNAL_LOGGER_IMPL_H_

#include <memory>

#include "base/macros.h"
#include "components/download/public/logger.h"

namespace base {
class Value;
}

namespace download {

// The internal Logger implementation.
class LoggerImpl : public Logger {
 public:
  LoggerImpl();
  ~LoggerImpl() override;

  std::unique_ptr<base::Value> GetServiceStatus() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(LoggerImpl);
};

}  // namespace download

#endif  // COMPONENTS_DOWNLOAD_INTERNAL_LOGGER_IMPL_H_
