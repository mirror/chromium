// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_TEST_MOCK_BACKGROUND_FETCH_DELEGATE_H_
#define CONTENT_TEST_MOCK_BACKGROUND_FETCH_DELEGATE_H_

#include <memory>
#include <string>
#include <vector>

#include "content/public/browser/background_fetch_delegate.h"

namespace content {

class MockBackgroundFetchDelegate : public BackgroundFetchDelegate {
 public:
  MockBackgroundFetchDelegate();
  ~MockBackgroundFetchDelegate() override;

  void DownloadUrl(const std::string& guid,
                   std::unique_ptr<DownloadUrlParameters> parameters,
                   scoped_refptr<BackgroundFetchRequestInfo> request) override;

  void OnDownloadStarted(
      const std::string& guid,
      const std::unique_ptr<const BackgroundFetchResponse> response) override;

  void OnDownloadUpdated(const std::string& guid,
                         uint64_t bytes_downloaded) override;

  void OnDownloadFailed(const std::string& guid, FailureReason reason) override;

  void OnDownloadSucceeded(const std::string& guid,
                           const base::FilePath& path,
                           uint64_t size) override;

  void SetDelegateClient(base::WeakPtr<Client> client) override;

 private:
  base::WeakPtr<Client> client_;
};

}  // namespace content

#endif  // CONTENT_TEST_MOCK_BACKGROUND_FETCH_DELEGATE_H_
