// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_PREFS_PERSISTENT_PREF_STORE_UNITTEST_H_
#define COMPONENTS_PREFS_PERSISTENT_PREF_STORE_UNITTEST_H_

class PersistentPrefStore;

// Verifies that the callback passed to store->CommitPendingWrite() runs
// asynchronously on the correct sequence. This function is meant to be reused
// to tests different PersistentPrefStore implementations.
void PersistentPrefStoreCommitPendingWriteCallbackTest(
    PersistentPrefStore* store);

#endif  // COMPONENTS_PREFS_PERSISTENT_PREF_STORE_UNITTEST_H_
