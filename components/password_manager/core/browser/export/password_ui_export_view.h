// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_EXPORT_PASSWORD_UI_EXPORT_VIEW_H_
#define COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_EXPORT_PASSWORD_UI_EXPORT_VIEW_H_

#include <string>

namespace password_manager {

// Interface for callbacks to the UI for exporting passwords.
class PasswordUIExportView {
 public:
  virtual ~PasswordUIExportView() = default;

  // Notify the UI that passwords have been exported to the destination.
  virtual void OnCompletedWritingToDestination(const std::string& error) = 0;
};

}  // namespace password_manager

#endif  // COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_EXPORT_PASSWORD_UI_EXPORT_VIEW_H_
