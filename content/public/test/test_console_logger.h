// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_TEST_TEST_CONSOLE_LOGGER_H_
#define CONTENT_PUBLIC_TEST_TEST_CONSOLE_LOGGER_H_

#include <string>
#include <vector>

#include "base/macros.h"
#include "content/public/browser/console_logger.h"

namespace content {

// This test class is designed so unit tests can inject itself in place of a
// standard ConsoleLogger, and intercept all requests for a RFH to log to the
// console.
class TestConsoleLogger : public ConsoleLogger {
 public:
  TestConsoleLogger();
  ~TestConsoleLogger() override;

  // ConsoleLogger:
  void LogInFrame(content::RenderFrameHost* render_frame_host,
                  content::ConsoleMessageLevel level,
                  const std::string& message) override;

  const std::vector<std::string>& messages() { return messages_; }

 private:
  std::vector<std::string> messages_;

  DISALLOW_COPY_AND_ASSIGN(TestConsoleLogger);
};

}  // namespace content

#endif  // CONTENT_PUBLIC_TEST_TEST_CONSOLE_LOGGER_H_
