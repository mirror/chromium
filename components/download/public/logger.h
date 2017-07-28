// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_DOWNLOAD_PUBLIC_LOGGER_H_
#define COMPONENTS_DOWNLOAD_PUBLIC_LOGGER_H_

#include <memory>

#include "base/macros.h"

namespace base {
class Value;
}

namespace download {

// A helper class to expose internals of the downloads system to a logging
// component and/or debug UI.
class Logger {
 public:
  virtual ~Logger() = default;

  virtual std::unique_ptr<base::Value> GetServiceStatus() = 0;

 protected:
  Logger() = default;

 private:
  DISALLOW_COPY_AND_ASSIGN(Logger);
};

}  // namespace download

#endif  // COMPONENTS_DOWNLOAD_PUBLIC_LOGGER_H_
