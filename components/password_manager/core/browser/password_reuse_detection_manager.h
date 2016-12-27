// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_PASSWORD_REUSE_DETECTION_MANAGER_H_
#define COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_PASSWORD_REUSE_DETECTION_MANAGER_H_

#include "base/macros.h"
#include "base/strings/string16.h"

namespace password_manager {

class PasswordManagerClient;

// Class for managing password reuse detection. Now it receives keystrokes and
// does nothing with them. TODO(crbug.com/657041): write other features of this
// class when they are implemented. This class is one per-tab.
class PasswordReuseDetectionManager {
 public:
  explicit PasswordReuseDetectionManager(PasswordManagerClient* client);
  ~PasswordReuseDetectionManager();

  void DidNavigateMainFrame();
  void OnKeyPressed(const base::string16& text);

 private:
  PasswordManagerClient* client_;
  base::string16 input_characters_;

  DISALLOW_COPY_AND_ASSIGN(PasswordReuseDetectionManager);
};

}  // namespace password_manager

#endif  // COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_PASSWORD_REUSE_DETECTION_MANAGER_H_
