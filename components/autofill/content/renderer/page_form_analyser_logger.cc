// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/autofill/content/renderer/page_form_analyser_logger.h"

#include <utility>

#include "third_party/WebKit/public/platform/WebString.h"

namespace autofill {

struct PageFormAnalyserLogger::LogEntry {
  // LogEntry(const std::string message, const std::vector<blink::WebNode>
  // nodes) : message(message), nodes(nodes) {}

  const std::string message;
  const std::vector<blink::WebNode> nodes;
};

PageFormAnalyserLogger::PageFormAnalyserLogger(blink::WebLocalFrame* frame)
    : frame_(frame) {}
PageFormAnalyserLogger::~PageFormAnalyserLogger() {}

void PageFormAnalyserLogger::Send(const std::string& message,
                                  ConsoleLevel level,
                                  const blink::WebNode& node) {
  Send(message, level, std::vector<blink::WebNode>{node});
}

void PageFormAnalyserLogger::Send(const std::string& message,
                                  ConsoleLevel level,
                                  const std::vector<blink::WebNode>& nodes) {
  node_buffer_[level].push_back(LogEntry{message, nodes});
}

void PageFormAnalyserLogger::Flush() {
  for (ConsoleLevel level : {kError, kWarning, kVerbose}) {
    for (LogEntry& entry : node_buffer_[level]) {
      std::string parameter_string;
      for (unsigned i = 0; i < entry.nodes.size(); ++i)
        parameter_string += " %o";
      frame_->AddMessageToConsole(blink::WebConsoleMessage(
          level,
          blink::WebString::FromUTF8("[DOM] " + entry.message +
                                     parameter_string),
          std::move(entry.nodes)));
    }
  }
  node_buffer_.clear();
}

}  // namespace autofill
