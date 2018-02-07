// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_WIN_UNINSTALL_APP_CONTROLLER_H_
#define CHROME_BROWSER_WIN_UNINSTALL_APP_CONTROLLER_H_

#include <memory>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/strings/string16.h"

class AutomationController;

class UninstallAppController {
 public:
  static void Launch(const base::string16& program_name);

 private:
  class AutomationControllerDelegate;

  explicit UninstallAppController(const base::string16& program_name);
  ~UninstallAppController();

  void OnUninstallFinished();

  // Allows the use of the UI Automation API.
  std::unique_ptr<AutomationController> automation_controller_;

  // Weak pointers are passed to the AutomationControllerDelegate so that it can
  // safely call us back from any thread.
  base::WeakPtrFactory<UninstallAppController> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(UninstallAppController);
};

#endif  // CHROME_BROWSER_WIN_UNINSTALL_APP_CONTROLLER_H_
