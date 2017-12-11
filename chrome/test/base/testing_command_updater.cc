// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/test/base/testing_command_updater.h"

#include "chrome/browser/command_observer.h"

TestingCommandUpdater::TestingCommandUpdater(CommandUpdaterDelegate* delegate)
    : impl_(delegate) {}

TestingCommandUpdater::~TestingCommandUpdater() {}

bool TestingCommandUpdater::SupportsCommand(int id) const {
  return impl_.SupportsCommand(id);
}

bool TestingCommandUpdater::IsCommandEnabled(int id) const {
  return impl_.IsCommandEnabled(id);
}

bool TestingCommandUpdater::ExecuteCommand(int id) {
  return impl_.ExecuteCommand(id);
}

bool TestingCommandUpdater::ExecuteCommandWithDisposition(
    int id, WindowOpenDisposition disposition) {
  return impl_.ExecuteCommandWithDisposition(id, disposition);
}

void TestingCommandUpdater::AddCommandObserver(
    int id, CommandObserver* observer) {
  impl_.AddCommandObserver(id, observer);
}

void TestingCommandUpdater::RemoveCommandObserver(
    int id, CommandObserver* observer) {
  impl_.RemoveCommandObserver(id, observer);
}

void TestingCommandUpdater::RemoveCommandObserver(
    CommandObserver* observer) {
  impl_.RemoveCommandObserver(observer);
}

bool TestingCommandUpdater::UpdateCommandEnabled(int id, bool state) {
  return impl_.UpdateCommandEnabled(id, state);
}
