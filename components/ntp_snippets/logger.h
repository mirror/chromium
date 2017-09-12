// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_NTP_SNIPPETS_LOGGER_H_
#define COMPONENTS_NTP_SNIPPETS_LOGGER_H_

#include <string>

#include "base/containers/circular_deque.h"
#include "base/time/time.h"

namespace ntp_snippets {

class Logger {
 public:
  Logger();
  ~Logger();

  void Log(const std::string& message);
  std::string GetHumanReadableLog() const;

 private:
  struct LogItem {
    std::string message;
    base::Time time;

    std::string ToString() const;
  };

  // New items should be pushed back.
  base::circular_deque<LogItem> logged_items_;
};

}  // namespace ntp_snippets

#endif  // COMPONENTS_NTP_SNIPPETS_LOGGER_H_
