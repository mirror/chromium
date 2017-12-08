// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_COMMAND_UPDATER_PROXY_H_
#define CHROME_BROWSER_COMMAND_UPDATER_PROXY_H_

#include "ui/base/window_open_disposition.h"

class CommandObserver;

// Interface that proxies calls to the underlying CommandUpdater.
class CommandUpdaterProxy {
 public:
  virtual bool SupportsCommand(int id) const = 0;
  virtual bool IsCommandEnabled(int id) const = 0;
  virtual bool ExecuteCommand(int id) = 0;
  virtual bool ExecuteCommandWithDispositionProxy(
      int id, WindowOpenDisposition disposition) = 0;
  virtual void AddCommandObserver(int id, CommandObserver* observer) = 0;
  virtual void RemoveCommandObserver(int id, CommandObserver* observer) = 0;
  virtual void RemoveCommandObserver(CommandObserver* observer) = 0;
  virtual bool UpdateCommandEnabled(int id, bool state) = 0;

 protected:
  virtual ~CommandUpdaterProxy() {}
};

#endif  // CHROME_BROWSER_COMMAND_UPDATER_PROXY_H_
