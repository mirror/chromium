// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_DEVTOOLS_PROTOCOL_PERMISSIONS_HANDLER_H_
#define CHROME_BROWSER_DEVTOOLS_PROTOCOL_PERMISSIONS_HANDLER_H_

#include "chrome/browser/devtools/protocol/permissions.h"

class PermissionsHandler : public protocol::Permissions::Backend {
 public:
  explicit PermissionsHandler(protocol::UberDispatcher* dispatcher);
  ~PermissionsHandler() override;


 private:
  DISALLOW_COPY_AND_ASSIGN(PermissionsHandler);
};

#endif  // CHROME_BROWSER_DEVTOOLS_PROTOCOL_PERMISSIONS_HANDLER_H_
