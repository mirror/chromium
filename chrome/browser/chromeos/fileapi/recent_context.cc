// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/fileapi/recent_context.h"

#include <utility>

namespace chromeos {

RecentContext::RecentContext() : is_valid_(false), profile_(nullptr) {}

RecentContext::RecentContext(storage::FileSystemContext* file_system_context,
                             const GURL& origin,
                             Profile* profile)
    : is_valid_(true),
      file_system_context_(file_system_context),
      origin_(origin),
      profile_(profile) {}

RecentContext::RecentContext(const RecentContext& other) = default;

RecentContext::~RecentContext() = default;

RecentContext& RecentContext::operator=(const RecentContext& other) = default;

}  // namespace chromeos
