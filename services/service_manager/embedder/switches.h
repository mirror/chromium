// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_SERVICE_MANAGER_EMBEDDER_SWITCHES_H_
#define SERVICES_SERVICE_MANAGER_EMBEDDER_SWITCHES_H_

#include "build/build_config.h"
#include "services/service_manager/embedder/service_manager_embedder_export.h"

namespace service_manager {
namespace switches {

SERVICE_MANAGER_EMBEDDER_EXPORT extern const char kEnableLogging[];
SERVICE_MANAGER_EMBEDDER_EXPORT extern const char kProcessType[];
SERVICE_MANAGER_EMBEDDER_EXPORT extern const char kProcessTypeServiceManager[];
SERVICE_MANAGER_EMBEDDER_EXPORT extern const char kProcessTypeService[];
SERVICE_MANAGER_EMBEDDER_EXPORT extern const char kSharedFiles[];

#if defined(OS_WIN)

// Prefetch arguments are used by the Windows prefetcher to disambiguate
// different execution modes (i.e. process types) of the same executable image
// so that different types of processes don't trample each others' prefetch
// behavior.
//
// Legal values are integers in the range [1, 8]. We reserve 8 to mean
// "whatever", and this will ultimately lead to processes with /prefetch:8
// having inconsistent behavior thus disabling prefetch in practice.
//
// TODO(rockot): Make it possible for embedders to override this argument on a
// per-service basis.
const char kDefaultServicePrefetchArgument[] = "/prefetch:8";

#endif  // defined(OS_WIN)

}  // namespace switches
}  // namespace service_manager

#endif  // SERVICES_SERVICE_MANAGER_EMBEDDER_SWITCHES_H_
