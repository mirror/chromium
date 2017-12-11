// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_COMMAND_UPDATER_IMPL_H_
#define CHROME_BROWSER_COMMAND_UPDATER_IMPL_H_

#include <memory>
#include <unordered_map>

#include "chrome/browser/command_updater.h"
#include "base/macros.h"
#include "ui/base/window_open_disposition.h"

class CommandObserver;
class CommandUpdaterDelegate;

////////////////////////////////////////////////////////////////////////////////
//
// CommandUpdaterImpl class
//
//   This object manages the enabled state of a set of commands. Observers
//   register to listen to changes in this state so they can update their
//   presentation.
//
class CommandUpdaterImpl : public CommandUpdater {
 public:
  // Create a CommandUpdaterImpl with |delegate| to handle the execution of
  // specific commands.
  explicit CommandUpdaterImpl(CommandUpdaterDelegate* delegate);
  ~CommandUpdaterImpl() override;

  // Returns true if the specified command ID is supported.
  bool SupportsCommand(int id) const override;

  // Returns true if the specified command ID is enabled. The command ID must be
  // supported by this updater.
  bool IsCommandEnabled(int id) const override;

  // Performs the action associated with this command ID using CURRENT_TAB
  // disposition.
  // Returns true if the command was executed (i.e. it is supported and is
  // enabled).
  bool ExecuteCommand(int id) override;

  // Performs the action associated with this command ID using the given
  // disposition.
  // Returns true if the command was executed (i.e. it is supported and is
  // enabled).
  bool ExecuteCommandWithDisposition(
      int id, WindowOpenDisposition disposition) override;

  // Adds an observer to the state of a particular command. If the command does
  // not exist, it is created, initialized to false.
  void AddCommandObserver(int id, CommandObserver* observer) override;

  // Removes an observer to the state of a particular command.
  void RemoveCommandObserver(int id, CommandObserver* observer) override;

  // Removes |observer| for all commands on which it's registered.
  void RemoveCommandObserver(CommandObserver* observer) override;

  // Notify all observers of a particular command that the command has been
  // enabled or disabled. If the command does not exist, it is created and
  // initialized to |state|. This function is very lightweight if the command
  // state has not changed.
  bool UpdateCommandEnabled(int id, bool state) override;

  void DisableAllCommands();

 private:
  // A piece of data about a command - whether or not it is enabled, and a list
  // of objects that observe the enabled state of this command.
  class Command;

  // Get a Command node for a given command ID, creating an entry if it doesn't
  // exist if desired.
  Command* GetCommand(int id, bool create);

  // The delegate is responsible for executing commands.
  CommandUpdaterDelegate* delegate_;

  // This is a map of command IDs to states and observer lists
  std::unordered_map<int, std::unique_ptr<Command>> commands_;

  DISALLOW_COPY_AND_ASSIGN(CommandUpdaterImpl);
};

#endif  // CHROME_BROWSER_COMMAND_UPDATER_IMPL_H_
