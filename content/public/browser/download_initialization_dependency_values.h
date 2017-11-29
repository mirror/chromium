// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Note that the embedder is welcome to persist these values across
// invocations of the browser, and possibly across browser versions.
// Thus individual dependencies may be deprecated and new dependencies added,
// but the values of particular dependencies should not be changed.

// Initialization dependencies.

// History database.
INITIALIZATION_DEPENDENCY(HISTORY_DB, 1)

// In-progress cache.
INITIALIZATION_DEPENDENCY(IN_PROGRESS_CACHE, 2)
