// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_TEST_MOCK_BACKGROUND_FETCH_DELEGATE_H_
#define CONTENT_TEST_MOCK_BACKGROUND_FETCH_DELEGATE_H_

#include <map>
#include <memory>
#include <string>

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

// Builder for creating a TestResponse object with the given data.
// MockBackgroundFetchDelegate will respond to the corresponding request based
// on this.
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

  // BackgroundFetchDelegate implementation:
  void DownloadUrl(const std::string& guid,
                   const std::string& method,
                   const GURL& url,
                   const net::NetworkTrafficAnnotationTag& traffic_annotation,
                   const net::HttpRequestHeaders& headers) override;

  void RegisterResponse(const std::string& url,
                        std::unique_ptr<TestResponse> response);

 private:
  std::map<std::string, std::unique_ptr<TestResponse>> url_responses_;
  std::map<std::string, std::unique_ptr<TestResponse>> guid_responses_;

  // Temporary directory in which successfully downloaded files will be stored.
  base::ScopedTempDir temp_directory_;
};

}  // namespace content

#endif  // CONTENT_TEST_MOCK_BACKGROUND_FETCH_DELEGATE_H_
