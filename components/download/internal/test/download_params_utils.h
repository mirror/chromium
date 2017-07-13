// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_DOWNLOAD_INTERNAL_TEST_DOWNLOAD_PARAMS_UTILS_H_
#define COMPONENTS_DOWNLOAD_INTERNAL_TEST_DOWNLOAD_PARAMS_UTILS_H_

#include <string>
#include <vector>

#include "components/download/internal/entry.h"
#include "net/traffic_annotation/network_traffic_annotation.h"

namespace download {
namespace test {

DownloadParams BuildBasicDownloadParams(
    const net::NetworkTrafficAnnotationTag& traffic_annotation);

}  // namespace test
}  // namespace download

#endif  // COMPONENTS_DOWNLOAD_INTERNAL_TEST_DOWNLOAD_PARAMS_UTILS_H_
