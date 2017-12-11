// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_TEST_BASE_TESTING_COMMAND_UPDATER_H_
#define CHROME_TEST_BASE_TESTING_COMMAND_UPDATER_H_

#include "chrome/browser/command_updater.h"
#include "chrome/browser/command_updater_delegate.h"
#include "chrome/browser/command_updater_impl.h"
#include "ui/base/window_open_disposition.h"

class CommandObserver;

// TODO(isandrk): Is this class needed anymore? AFAICT CommandUpdaterImpl can be
// used in all places.
class TestingCommandUpdater : public CommandUpdater {
 public:
  explicit TestingCommandUpdater(CommandUpdaterDelegate* delegate);
  ~TestingCommandUpdater() override;

  bool SupportsCommand(int id) const override;
  bool IsCommandEnabled(int id) const override;
  bool ExecuteCommand(int id) override;
  bool ExecuteCommandWithDisposition(int id, WindowOpenDisposition disposition)
      override;
  void AddCommandObserver(int id, CommandObserver* observer) override;
  void RemoveCommandObserver(int id, CommandObserver* observer) override;
  void RemoveCommandObserver(CommandObserver* observer) override;
  bool UpdateCommandEnabled(int id, bool state) override;

 private:
  CommandUpdaterImpl impl_;
};

#endif  // CHROME_TEST_BASE_TESTING_COMMAND_UPDATER_H_
