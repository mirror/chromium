// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_TOOLS_CERT_MAPPER_WRITE_RESULTS_H_
#define NET_TOOLS_CERT_MAPPER_WRITE_RESULTS_H_

#include "base/files/file_path.h"

namespace net {

class Metrics;

void WriteResultsToDirectory(const Metrics& metrics,
                             const base::FilePath& dir_path);

}  // namespace net

#endif  // NET_TOOLS_CERT_MAPPER_WRITE_RESULTS_H_
