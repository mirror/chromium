// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/download/public/background_service/test/mock_client.h"

#include "storage/browser/blob/blob_data_handle.h"

namespace download {
namespace test {

MockClient::MockClient() = default;
MockClient::~MockClient() = default;

void MockClient::GetUploadData(const std::string& guid,
                               GetUploadDataCallback callback) {
  std::unique_ptr<storage::BlobDataHandle> handle;
  std::move(callback).Run(std::move(handle));
}

}  // namespace test
}  // namespace download
