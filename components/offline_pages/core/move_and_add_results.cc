// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/move_and_add_results.h"

namespace offline_pages {

MoveAndAddResults::MoveAndAddResults()
    : move_result_(SavePageResult::CANCELLED), download_id_(0LL) {}

MoveAndAddResults::~MoveAndAddResults() {}

}  // namespace offline_pages
