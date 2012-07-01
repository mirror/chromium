// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Constants used by SyncScheduler when polling servers for updates.

#ifndef SYNC_INTERNAL_API_PUBLIC_ENGINE_POLLING_CONSTANTS_H_
#define SYNC_INTERNAL_API_PUBLIC_ENGINE_POLLING_CONSTANTS_H_
#pragma once

namespace syncer {

extern const int64 kDefaultShortPollIntervalSeconds;
extern const int64 kDefaultLongPollIntervalSeconds;
extern const int64 kMaxBackoffSeconds;
extern const int kBackoffRandomizationFactor;

}  // namespace syncer

#endif  // SYNC_INTERNAL_API_PUBLIC_ENGINE_POLLING_CONSTANTS_H_
