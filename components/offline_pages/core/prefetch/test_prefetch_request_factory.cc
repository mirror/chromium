// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/prefetch/test_prefetch_request_factory.h"

#include <memory>
#include <string>

#include "base/memory/ptr_util.h"
#include "components/offline_pages/core/prefetch/generate_page_bundle_request.h"
#include "components/offline_pages/core/prefetch/get_operation_request.h"

namespace offline_pages {
namespace {
version_info::Channel kChannel = version_info::Channel::UNKNOWN;
const char kUserAgent[] = "Chrome/57.0.2987.133";
}  // namespace

// static
TestNetworkRequestFactory* TestNetworkRequestFactory::Create() {
  scoped_refptr<net::TestURLRequestContextGetter> request_context =
      new net::TestURLRequestContextGetter(base::ThreadTaskRunnerHandle::Get());
  TestNetworkRequestFactory* result =
      new TestNetworkRequestFactory(request_context.get());
  result->request_context = request_context;
  return result;
}

TestNetworkRequestFactory::TestNetworkRequestFactory(
    net::TestURLRequestContextGetter* request_context)
    : PrefetchNetworkRequestFactoryImpl(request_context, kChannel, kUserAgent) {
}

TestNetworkRequestFactory::~TestNetworkRequestFactory() = default;

}  // namespace offline_pages
