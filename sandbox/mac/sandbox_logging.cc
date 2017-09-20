// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "sandbox/mac/sandbox_logging.h"

#include <asl.h>
#include <stdarg.h>
#include <stdio.h>
#include <syslog.h>
#include <unistd.h>

#include <limits>
#include <string>

namespace SandboxLogging {

namespace {

void send_asl_log(LogLevel level, const char* message) {
  class ASLClient {
   public:
    explicit ASLClient()
        : client_(asl_open(nullptr,
                           "com.apple.console",
                           ASL_OPT_STDERR | ASL_OPT_NO_DELAY)) {}
    ~ASLClient() { asl_close(client_); }

    aslclient get() const { return client_; }

    ASLClient(const ASLClient&) = delete;
    ASLClient& operator=(const ASLClient&) = delete;

   private:
    aslclient client_;
  } asl_client;

  class ASLMessage {
   public:
    ASLMessage() : message_(asl_new(ASL_TYPE_MSG)) {}
    ~ASLMessage() { asl_free(message_); }

    aslmsg get() const { return message_; }

    ASLMessage(const ASLMessage&) = delete;
    ASLMessage& operator=(const ASLMessage&) = delete;

   private:
    aslmsg message_;
  } asl_message;

  // By default, messages are only readable by the admin group. Explicitly
  // make them readable by the user generating the messages.
  char euid_string[12];
  snprintf(euid_string, sizeof(euid_string) / sizeof(euid_string[0]), "%d",
           geteuid());
  asl_set(asl_message.get(), ASL_KEY_READ_UID, euid_string);

  std::string asl_level_string;
  switch (level) {
    case LogLevel::FATAL:
      asl_level_string = ASL_STRING_CRIT;
      break;
    case LogLevel::ERR:
      asl_level_string = ASL_STRING_ERR;
      break;
    case LogLevel::WARN:
      asl_level_string = ASL_STRING_WARNING;
      break;
    case LogLevel::INFO:
    default:
      asl_level_string = ASL_STRING_INFO;
      break;
  }

  asl_set(asl_message.get(), ASL_KEY_LEVEL, asl_level_string.c_str());
  asl_set(asl_message.get(), ASL_KEY_MSG, message);
  asl_send(asl_client.get(), asl_message.get());
}

void do_logging(LogLevel level, const char* fmt, va_list args) {
  char message[4096];
  int ret = vsnprintf(message, sizeof(message), fmt, args);

  if (ret < 0) {
    return;
  }

  if (static_cast<int>(static_cast<unsigned long>(ret)) != ret)
    return;

  bool truncated = static_cast<unsigned long>(ret) > sizeof(message) - 1;
  send_asl_log(level, message);

  if (truncated)
    send_asl_log(level, "warning: previous log message truncated");
}

}  // namespace

void log(LogLevel level, const char* fmt, ...) {
  va_list args;
  va_start(args, fmt);
  do_logging(level, fmt, args);
  va_end(args);

  if (level == LogLevel::FATAL)
    ABORT();
}

}  // namespace SandboxLogging
