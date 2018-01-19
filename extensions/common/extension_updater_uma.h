// Copyright (c) 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_COMMON_EXTENSION_UPDATER_UMA_H_
#define EXTENSIONS_COMMON_EXTENSION_UPDATER_UMA_H_

namespace extensions {

enum ExtensionUpdaterUpdateCall {
  UPDATE_CALL = 0,

  UPDATE_CALL_COUNT
};

enum ExtensionUpdaterUpdateFound {
  UPDATE_FOUND = 0,

  UPDATE_FOUND_COUNT
};

enum ExtensionUpdaterUpdateResult {
  NO_UPDATE = 0,
  UPDATE_SUCCESS,
  UPDATE_ERROR,

  UPDATE_RESULT_COUNT
};

}  // namespace extensions

#endif  // EXTENSIONS_COMMON_EXTENSION_UPDATER_UMA_H_
