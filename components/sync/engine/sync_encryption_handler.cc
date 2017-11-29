// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync/engine/sync_encryption_handler.h"

namespace syncer {

SyncEncryptionHandler::Observer::Observer() = default;
SyncEncryptionHandler::Observer::~Observer() = default;

SyncEncryptionHandler::SyncEncryptionHandler() = default;
SyncEncryptionHandler::~SyncEncryptionHandler() = default;

// Static.
ModelTypeSet SyncEncryptionHandler::SensitiveTypes() {
  ModelTypeSet types;
  types.Put(PASSWORDS);  // Has its own encryption, but include it anyway.
  types.Put(WIFI_CREDENTIALS);
  return types;
}

}  // namespace syncer
