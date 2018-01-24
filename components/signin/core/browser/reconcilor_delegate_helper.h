// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#ifndef COMPONENTS_SIGNIN_CORE_BROWSER_RECONCILOR_DELEGATE_HELPER_H_
#define COMPONENTS_SIGNIN_CORE_BROWSER_RECONCILOR_DELEGATE_HELPER_H_

#include "base/macros.h"

class ReconcilorDelegateHelper {
 public:
  ReconcilorDelegateHelper();
  virtual ~ReconcilorDelegateHelper();
  virtual void ForceUserOnlineSignIn();
  virtual void AttemptUserExit();

 private:
  DISALLOW_COPY_AND_ASSIGN(ReconcilorDelegateHelper);
};

#endif  // COMPONENTS_SIGNIN_CORE_BROWSER_RECONCILOR_DELEGATE_HELPER_H_
