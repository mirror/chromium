// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/download/public/test/mock_client.h"

namespace download {
namespace test {

MockClient::MockClient() = default;
MockClient::~MockClient() = default;

void MockClient::GetUploadData(const std::string& guid,
                               GetUploadDataCallback callback) {
  std::move(callback).Run(std::string());
}

}  // namespace test
}  // namespace download
