// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_DEVTOOLS_DEVTOOLS_BROWSER_PROTOCOL_HANDLER_H_
#define CHROME_BROWSER_DEVTOOLS_DEVTOOLS_BROWSER_PROTOCOL_HANDLER_H_

#include <memory>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "chrome/browser/devtools/devtools_protocol.h"

class DevToolsBrowserProtocolHandler {
 public:
  DevToolsBrowserProtocolHandler();
  ~DevToolsBrowserProtocolHandler();

  std::unique_ptr<base::DictionaryValue> HandleCommand(
      int id,
      const std::string& method,
      const base::DictionaryValue& params);

 private:
  DISALLOW_COPY_AND_ASSIGN(DevToolsBrowserProtocolHandler);
};

#endif  // CHROME_BROWSER_DEVTOOLS_DEVTOOLS_BROWSER_PROTOCOL_HANDLER_H_
