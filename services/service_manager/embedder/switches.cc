// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/service_manager/embedder/switches.h"

namespace service_manager {
namespace switches {

// Controls whether console logging is enabled and optionally configures where
// it's routed.
const char kEnableLogging[] = "enable-logging";

// Indicates the type of process to run. This may be "service-manager",
// "service", or any other arbitrary value supported by the embedder.
const char kProcessType[] = "type";

// The value of the |kProcessType| switch which tells the executable to assume
// the role of a standalone Service Manager instance.
const char kProcessTypeServiceManager[] = "service-manager";

// The value of the |kProcessType| switch which tells the executable to assume
// the role of a service instance. The name of the embedded service to execute
// must be given by the |kServiceName| switch.
const char kProcessTypeService[] = "service";

// If |kProcessType| is "service" this indicates the name of the embedded
// service to be run in this process.
const char kServiceName[] = "service-name";

// Describes the file descriptors passed to a child process in the following
// list format:
//
//     <file_id>:<descriptor_id>,<file_id>:<descriptor_id>,...
//
// where <file_id> is an ID string from the manifest of the service being
// launched and <descriptor_id> is the numeric identifier of the descriptor for
// the child process can use to retrieve the file descriptor from the
// global descriptor table.
const char kSharedFiles[] = "shared-files";

}  // namespace switches
}  // namespace service_manager
