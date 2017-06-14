// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/cocoa/simple_message_box_cocoa.h"
#include "chrome/browser/ui/simple_message_box.h"
#include "chrome/browser/ui/views/simple_message_box_views.h"
#include "ui/base/material_design/material_design_controller.h"

namespace {

chrome::MessageBoxResult ShowMessageBoxImpl(
    gfx::NativeWindow parent,
    const base::string16& title,
    const base::string16& message,
    chrome::MessageBoxType type,
    const base::string16& yes_text,
    const base::string16& no_text,
    const base::string16& checkbox_text) {
  if (ui::MaterialDesignController::IsSecondaryUiMaterial()) {
    return chrome::ShowMessageBoxViews(parent, title, message, type, yes_text,
                                       no_text, checkbox_text);
  } else {
    return chrome::ShowMessageBoxCocoa(parent, title, message, type,
                                       checkbox_text);
  }
}
}

namespace chrome {

void ShowWarningMessageBox(gfx::NativeWindow parent,
                           const base::string16& title,
                           const base::string16& message) {
  ShowMessageBoxImpl(parent, title, message, MESSAGE_BOX_TYPE_WARNING,
                     base::string16(), base::string16(), base::string16());
}

bool ShowWarningMessageBoxWithCheckbox(gfx::NativeWindow parent,
                                       const base::string16& title,
                                       const base::string16& message,
                                       const base::string16& checkbox_text) {
  return ShowMessageBoxImpl(parent, title, message, MESSAGE_BOX_TYPE_WARNING,
                            base::string16(), base::string16(),
                            checkbox_text) == MESSAGE_BOX_RESULT_YES;
}

MessageBoxResult ShowQuestionMessageBox(gfx::NativeWindow parent,
                                        const base::string16& title,
                                        const base::string16& message) {
  return ShowMessageBoxImpl(parent, title, message, MESSAGE_BOX_TYPE_QUESTION,
                            base::string16(), base::string16(),
                            base::string16());
}

bool CloseMessageBoxForTest(bool accept) {
  NOTIMPLEMENTED();
  return false;
}
}
