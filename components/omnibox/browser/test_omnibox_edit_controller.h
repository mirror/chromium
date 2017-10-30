// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/omnibox/browser/omnibox_edit_controller.h"

#include "components/omnibox/browser/omnibox_edit_controller.h"
#include "components/toolbar/test_toolbar_model.h"

class TestOmniboxEditController : public OmniboxEditController {
 public:
  TestOmniboxEditController() {}

  // OmniboxEditController:
  void OnInputInProgress(bool in_progress) override {}
  void OnChanged() override {}
  ToolbarModel* GetToolbarModel() override;
  const ToolbarModel* GetToolbarModel() const override;

 private:
  TestToolbarModel toolbar_model_;

  DISALLOW_COPY_AND_ASSIGN(TestOmniboxEditController);
};
