// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/public/test/test_storage_partition.h"

namespace content {

TestStoragePartition::TestStoragePartition() {}
TestStoragePartition::~TestStoragePartition() {}

void TestStoragePartition::ClearDataForOrigin(
    uint32_t remove_mask,
    uint32_t quota_storage_remove_mask,
    const GURL& storage_origin,
    net::URLRequestContextGetter* rq_context,
    const base::Closure& callback) {}

void TestStoragePartition::ClearData(
    uint32_t remove_mask,
    uint32_t quota_storage_remove_mask,
    const GURL& storage_origin,
    const OriginMatcherFunction& origin_matcher,
    const base::Time begin,
    const base::Time end,
    const base::Closure& callback) {}

void TestStoragePartition::ClearData(
    uint32_t remove_mask,
    uint32_t quota_storage_remove_mask,
    const OriginMatcherFunction& origin_matcher,
    const CookieMatcherFunction& cookie_matcher,
    const base::Time begin,
    const base::Time end,
    const base::Closure& callback) {}

void TestStoragePartition::ClearHttpAndMediaCaches(
    const base::Time begin,
    const base::Time end,
    const base::Callback<bool(const GURL&)>& url_matcher,
    const base::Closure& callback) {}

void TestStoragePartition::Flush() {}

void TestStoragePartition::ClearBluetoothAllowedDevicesMapForTesting() {}

}  // namespace content
