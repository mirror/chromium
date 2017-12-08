// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_TEST_BASE_TESTING_COMMAND_UPDATER_PROXY_H_
#define CHROME_TEST_BASE_TESTING_COMMAND_UPDATER_PROXY_H_

#include "chrome/browser/command_updater.h"
#include "chrome/browser/command_updater_delegate.h"
#include "chrome/browser/command_updater_proxy.h"
#include "ui/base/window_open_disposition.h"

class CommandObserver;

class TestingCommandUpdaterProxy : public CommandUpdaterProxy {
 public:
  explicit TestingCommandUpdaterProxy(CommandUpdaterDelegate* delegate);
  ~TestingCommandUpdaterProxy() override;

  bool SupportsCommand(int id) const override;
  bool IsCommandEnabled(int id) const override;
  bool ExecuteCommand(int id) override;
  bool ExecuteCommandWithDispositionProxy(
      int id, WindowOpenDisposition disposition) override;
  void AddCommandObserver(int id, CommandObserver* observer) override;
  void RemoveCommandObserver(int id, CommandObserver* observer) override;
  void RemoveCommandObserver(CommandObserver* observer) override;
  bool UpdateCommandEnabled(int id, bool state) override;

 private:
  CommandUpdater command_updater_;
};

#endif  // CHROME_TEST_BASE_TESTING_COMMAND_UPDATER_PROXY_H_
