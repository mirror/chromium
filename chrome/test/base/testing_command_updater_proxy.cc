// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/test/base/testing_command_updater_proxy.h"

#include "chrome/browser/command_observer.h"

TestingCommandUpdaterProxy::TestingCommandUpdaterProxy(
    CommandUpdaterDelegate* delegate) : command_updater_(delegate) {}

TestingCommandUpdaterProxy::~TestingCommandUpdaterProxy() {}

bool TestingCommandUpdaterProxy::SupportsCommand(int id) const {
  return command_updater_.SupportsCommand(id);
}

bool TestingCommandUpdaterProxy::IsCommandEnabled(int id) const {
  return command_updater_.IsCommandEnabled(id);
}

bool TestingCommandUpdaterProxy::ExecuteCommand(int id) {
  return command_updater_.ExecuteCommand(id);
}

bool TestingCommandUpdaterProxy::ExecuteCommandWithDispositionProxy(
    int id, WindowOpenDisposition disposition) {
  return command_updater_.ExecuteCommandWithDisposition(id, disposition);
}

void TestingCommandUpdaterProxy::AddCommandObserver(
    int id, CommandObserver* observer) {
  command_updater_.AddCommandObserver(id, observer);
}

void TestingCommandUpdaterProxy::RemoveCommandObserver(
    int id, CommandObserver* observer) {
  command_updater_.RemoveCommandObserver(id, observer);
}

void TestingCommandUpdaterProxy::RemoveCommandObserver(
    CommandObserver* observer) {
  command_updater_.RemoveCommandObserver(observer);
}

bool TestingCommandUpdaterProxy::UpdateCommandEnabled(int id, bool state) {
  command_updater_.UpdateCommandEnabled(id, state);
  return true;
}
