// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_BACKGROUND_FETCH_MOCK_BACKGROUND_FETCH_DELEGATE_H_
#define CONTENT_BROWSER_BACKGROUND_FETCH_MOCK_BACKGROUND_FETCH_DELEGATE_H_

#include <map>
#include <memory>
#include <set>
#include <string>

#include "base/files/scoped_temp_dir.h"
#include "content/public/browser/background_fetch_delegate.h"
#include "url/gurl.h"

namespace net {
class HttpResponseHeaders;
}

namespace content {

class MockBackgroundFetchDelegate : public BackgroundFetchDelegate {
 public:
  // Structure encapsulating the data for a injected OnDownloadComplete
  // response. Should be created directly.
  struct TestCompletedResponse {
    TestCompletedResponse(bool succeeded, const std::string& data);

    bool succeeded;
    std::string data;

   private:
    DISALLOW_COPY_AND_ASSIGN(TestCompletedResponse);
  };

  // Builder for creating an injected OnDownloadStarted response.
  class TestStartedResponseBuilder {
   public:
    explicit TestStartedResponseBuilder(int response_code);

    TestStartedResponseBuilder& AddResponseHeader(const std::string& name,
                                                  const std::string& value);

    // Finalizes the builder and invalidates the underlying response.
    scoped_refptr<net::HttpResponseHeaders> Build();

   private:
    scoped_refptr<net::HttpResponseHeaders> headers_;

    DISALLOW_COPY_AND_ASSIGN(TestStartedResponseBuilder);
  };

  MockBackgroundFetchDelegate();
  ~MockBackgroundFetchDelegate() override;

  // BackgroundFetchDelegate implementation:
  void DownloadUrl(const std::string& guid,
                   const std::string& method,
                   const GURL& url,
                   const net::NetworkTrafficAnnotationTag& traffic_annotation,
                   const net::HttpRequestHeaders& headers) override;

  void RegisterStartedResponse(
      const GURL& url,
      scoped_refptr<net::HttpResponseHeaders> response_headers);

  void RegisterCompletedResponse(
      const GURL& url,
      std::unique_ptr<TestCompletedResponse> response);

 private:
  // Single-use OnDownloadStarted responses registered for specific URLs.
  std::map<GURL, scoped_refptr<net::HttpResponseHeaders>> started_responses_;

  // Single-use OnDownloadComplete responses registered for specific URLs.
  std::map<GURL, std::unique_ptr<TestCompletedResponse>> completed_responses_;

  // Single-use download GUIDs of requests awaiting a started response.
  std::map<GURL, std::string> requests_awaiting_start_;

  // Single-use download GUIDs of requests awaiting a completed response.
  std::map<GURL, std::string> requests_awaiting_complete_;

  // GUIDs that have been registered via DownloadUrl and thus cannot be reused.
  std::set<std::string> seen_guids_;

  // Temporary directory in which successfully downloaded files will be stored.
  base::ScopedTempDir temp_directory_;

  DISALLOW_COPY_AND_ASSIGN(MockBackgroundFetchDelegate);
};

}  // namespace content

#endif  // CONTENT_BROWSER_BACKGROUND_FETCH_MOCK_BACKGROUND_FETCH_DELEGATE_H_
