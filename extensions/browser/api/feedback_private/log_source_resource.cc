// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/browser/api/feedback_private/log_source_resource.h"

#include "base/lazy_instance.h"

namespace extensions {

// For managing API resources of type LogSourceResource.
static base::LazyInstance<BrowserContextKeyedAPIFactory<
    ApiResourceManager<LogSourceResource>>>::DestructorAtExit
    g_log_source_resource_factory = LAZY_INSTANCE_INITIALIZER;

// static
template <>
BrowserContextKeyedAPIFactory<ApiResourceManager<LogSourceResource>>*
ApiResourceManager<LogSourceResource>::GetFactoryInstance() {
  return g_log_source_resource_factory.Pointer();
}

LogSourceResource::LogSourceResource(
    const std::string& extension_id,
    std::unique_ptr<system_logs::SystemLogsSource> source,
    const UnregisterCallback& callback)
    : ApiResource(extension_id),
      source_(source.release()),
      callback_(callback),
      resource_id_(0) {}

LogSourceResource::~LogSourceResource() {
  if (!callback_.is_null() && resource_id_)
    callback_.Run(resource_id_);
}

}  // namespace extensions
