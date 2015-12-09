// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_EXTENSIONS_API_INPUT_IME_INPUT_IME_API_CHROMEOS_H_
#define CHROME_BROWSER_EXTENSIONS_API_INPUT_IME_INPUT_IME_API_CHROMEOS_H_

#include "extensions/browser/extension_function.h"

namespace extensions {

class InputImeSetCompositionFunction : public SyncExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("input.ime.setComposition",
                             INPUT_IME_SETCOMPOSITION)

 protected:
  ~InputImeSetCompositionFunction() override {}

  // ExtensionFunction:
  bool RunSync() override;
};

class InputImeClearCompositionFunction : public SyncExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("input.ime.clearComposition",
                             INPUT_IME_CLEARCOMPOSITION)

 protected:
  ~InputImeClearCompositionFunction() override {}

  // ExtensionFunction:
  bool RunSync() override;
};

class InputImeCommitTextFunction : public SyncExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("input.ime.commitText", INPUT_IME_COMMITTEXT)

 protected:
  ~InputImeCommitTextFunction() override {}

  // ExtensionFunction:
  bool RunSync() override;
};

class InputImeSetCandidateWindowPropertiesFunction
    : public SyncExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("input.ime.setCandidateWindowProperties",
                             INPUT_IME_SETCANDIDATEWINDOWPROPERTIES)

 protected:
  ~InputImeSetCandidateWindowPropertiesFunction() override {}

  // ExtensionFunction:
  bool RunSync() override;
};

class InputImeSetCandidatesFunction : public SyncExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("input.ime.setCandidates", INPUT_IME_SETCANDIDATES)

 protected:
  ~InputImeSetCandidatesFunction() override {}

  // ExtensionFunction:
  bool RunSync() override;
};

class InputImeSetCursorPositionFunction : public SyncExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("input.ime.setCursorPosition",
                             INPUT_IME_SETCURSORPOSITION)

 protected:
  ~InputImeSetCursorPositionFunction() override {}

  // ExtensionFunction:
  bool RunSync() override;
};

class InputImeSetMenuItemsFunction : public SyncExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("input.ime.setMenuItems", INPUT_IME_SETMENUITEMS)

 protected:
  ~InputImeSetMenuItemsFunction() override {}

  // ExtensionFunction:
  bool RunSync() override;
};

class InputImeUpdateMenuItemsFunction : public SyncExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("input.ime.updateMenuItems",
                             INPUT_IME_UPDATEMENUITEMS)

 protected:
  ~InputImeUpdateMenuItemsFunction() override {}

  // ExtensionFunction:
  bool RunSync() override;
};

class InputImeDeleteSurroundingTextFunction : public SyncExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("input.ime.deleteSurroundingText",
                             INPUT_IME_DELETESURROUNDINGTEXT)
 protected:
  ~InputImeDeleteSurroundingTextFunction() override {}

  // ExtensionFunction:
  bool RunSync() override;
};

class InputImeSendKeyEventsFunction : public AsyncExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("input.ime.sendKeyEvents",
                             INPUT_IME_SENDKEYEVENTS)

 protected:
  ~InputImeSendKeyEventsFunction() override {}

  // ExtensionFunction:
  bool RunAsync() override;
};

class InputImeHideInputViewFunction : public AsyncExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("input.ime.hideInputView",
                             INPUT_IME_HIDEINPUTVIEW)

 protected:
  ~InputImeHideInputViewFunction() override {}

  // ExtensionFunction:
  bool RunAsync() override;
};

}  // namespace extensions

#endif  // CHROME_BROWSER_EXTENSIONS_API_INPUT_IME_INPUT_IME_API_CHROMEOS_H_
