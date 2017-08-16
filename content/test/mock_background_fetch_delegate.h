// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_TEST_MOCK_BACKGROUND_FETCH_DELEGATE_H_
#define CONTENT_TEST_MOCK_BACKGROUND_FETCH_DELEGATE_H_

#include <memory>
#include <string>
#include <unordered_map>

#include "base/files/scoped_temp_dir.h"
#include "content/public/browser/background_fetch_delegate.h"
#include "url/gurl.h"

namespace net {
class HttpResponseHeaders;
}

namespace content {

// Structure encapsulating the data for a injected response. Should only be
// created by the builder, which also defines the ownership semantics.
struct TestResponse {
  TestResponse();
  ~TestResponse();

  bool succeeded_;
  scoped_refptr<net::HttpResponseHeaders> headers;
  std::string data;
};

// Builder for creating a TestResponse object with the given data. The faked
// download manager will respond to the corresponding request based on this.
class TestResponseBuilder {
 public:
  explicit TestResponseBuilder(bool succeeded, int response_code);
  ~TestResponseBuilder();

  TestResponseBuilder& AddResponseHeader(const std::string& name,
                                         const std::string& value);
  TestResponseBuilder& SetResponseData(std::string data);

  // Finalizes the builder and invalidates the underlying response.
  std::unique_ptr<TestResponse> Build();

 private:
  std::unique_ptr<TestResponse> response_;

  DISALLOW_COPY_AND_ASSIGN(TestResponseBuilder);
};

class MockBackgroundFetchDelegate : public BackgroundFetchDelegate {
 public:
  MockBackgroundFetchDelegate();
  ~MockBackgroundFetchDelegate() override;

  void DownloadUrl(const std::string& guid,
                   std::unique_ptr<DownloadUrlParameters> parameters,
                   scoped_refptr<BackgroundFetchRequestInfo> request) override;

  void OnDownloadStarted(
      const std::string& guid,
      std::unique_ptr<const BackgroundFetchResponse> response) override;

  void OnDownloadUpdated(const std::string& guid,
                         uint64_t bytes_downloaded) override;

  void OnDownloadFailed(const std::string& guid, FailureReason reason) override;

  void OnDownloadSucceeded(const std::string& guid,
                           const base::FilePath& path,
                           uint64_t size) override;

  void SetDelegateClient(base::WeakPtr<Client> client) override;

  void RegisterResponse(const std::string& url,
                        std::unique_ptr<TestResponse> response);

 private:
  base::WeakPtr<Client> client_;

  std::unordered_map<std::string, std::unique_ptr<TestResponse>> url_responses_;
  std::unordered_map<std::string, std::unique_ptr<TestResponse>>
      guid_responses_;

  // Temporary directory in which successfully downloaded files will be stored.
  base::ScopedTempDir temp_directory_;
};

}  // namespace content

#endif  // CONTENT_TEST_MOCK_BACKGROUND_FETCH_DELEGATE_H_
