// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/passwords/password_ui_view_mock.h"

explicit MockPasswordUIView::MockPasswordUIView(Profile* profile)
    : profile_(profile), password_manager_presenter_(this) {
  password_manager_presenter_.Initialize();
}

MockPasswordUIView::~MockPasswordUIView() = default;
