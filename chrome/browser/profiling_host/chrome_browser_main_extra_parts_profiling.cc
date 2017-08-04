// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/profiling_host/chrome_browser_main_extra_parts_profiling.h"

#include "chrome/browser/profiling_host/profiling_process_host.h"

ChromeBrowserMainExtraPartsProfiling::ChromeBrowserMainExtraPartsProfiling() =
    default;
ChromeBrowserMainExtraPartsProfiling::~ChromeBrowserMainExtraPartsProfiling() =
    default;

void ChromeBrowserMainExtraPartsProfiling::ServiceManagerConnectionStarted(
    content::ServiceManagerConnection* connection) {
  profiling::ProfilingProcessHost::EnsureStarted(connection);
}
