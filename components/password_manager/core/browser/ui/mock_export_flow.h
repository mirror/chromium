// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_UI_MOCK_EXPORT_FLOW_H_
#define COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_UI_MOCK_EXPORT_FLOW_H_

#include "base/macros.h"
#include "components/password_manager/core/browser/ui/export_flow.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace password_manager {

class MockExportFlow : public ExportFlow {
 public:
  MockExportFlow();
  ~MockExportFlow() override;

  // ExportFlow:
  MOCK_METHOD0(Store, void());

 private:
  DISALLOW_COPY_AND_ASSIGN(MockExportFlow);
};

}  // namespace password_manager

#endif  // COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_UI_EXPORT_FLOW_H_
