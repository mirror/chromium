// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/ntp_snippets/remote/cached_image_fetcher.h"

#include <memory>

#include "base/bind.h"
#include "base/callback.h"
#include "base/files/file_path.h"
#include "base/files/scoped_temp_dir.h"
#include "base/memory/ptr_util.h"
#include "base/test/test_mock_time_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "components/image_fetcher/core/image_decoder.h"
#include "components/image_fetcher/core/image_fetcher.h"
#include "components/image_fetcher/core/image_fetcher_impl.h"
#include "components/ntp_snippets/remote/proto/ntp_snippets.pb.h"
#include "components/ntp_snippets/remote/remote_suggestions_database.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/testing_pref_service.h"
#include "net/url_request/test_url_fetcher_factory.h"
#include "net/url_request/url_request_test_util.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/gfx/image/image.h"

using testing::_;
using testing::Eq;

namespace ntp_snippets {

namespace {

const std::string kImageData = "data";
const GURL kImageURL("http://image.test/test.png");
const std::string kSnippetID("http://localhost");
const ContentSuggestion::ID kContentSuggestionsID(
    Category::FromKnownCategory(KnownCategories::ARTICLES),
    kSnippetID);

class MockImageDecoder : public image_fetcher::ImageDecoder {
 public:
  MOCK_METHOD3(DecodeImage,
               void(const std::string& image_data,
                    const gfx::Size& desired_image_frame_size,
                    const image_fetcher::ImageDecodedCallback& callback));
};

}  // namespace

class CachedImageFetcherTest : public testing::Test {
 public:
  CachedImageFetcherTest()
      : fake_url_fetcher_factory_(
            base::MakeUnique<net::FakeURLFetcherFactory>(nullptr)),
        mock_task_runner_(new base::TestMockTimeTaskRunner()),
        mock_task_runner_handle_(mock_task_runner_),
        pref_service_(base::MakeUnique<TestingPrefServiceSimple>()) {
    EXPECT_TRUE(database_dir_.CreateUniqueTempDir());

    RequestThrottler::RegisterProfilePrefs(pref_service_->registry());
    // Explicitly destroy any existing database first, so it releases the lock
    // on the file.
    database_.reset();
    database_.reset(new RemoteSuggestionsDatabase(database_dir_.GetPath(),
                                                  mock_task_runner_));
    request_context_getter_ = scoped_refptr<net::TestURLRequestContextGetter>(
        new net::TestURLRequestContextGetter(mock_task_runner_.get()));

    mock_image_decoder_ = new MockImageDecoder;
    cached_image_fetcher_ = base::MakeUnique<ntp_snippets::CachedImageFetcher>(
        base::MakeUnique<image_fetcher::ImageFetcherImpl>(
            std::unique_ptr<image_fetcher::ImageDecoder>(mock_image_decoder_),
            request_context_getter_.get()),
        pref_service_.get(), database_.get());
    FastForwardUntilNoTasksRemain();
    EXPECT_TRUE(database_->IsInitialized());
  }

  void FastForwardUntilNoTasksRemain() {
    mock_task_runner_->FastForwardUntilNoTasksRemain();
  }

  void FetchImage() {
    cached_image_fetcher_->FetchSuggestionImage(
        kContentSuggestionsID, kImageURL,
        base::Bind(&CachedImageFetcherTest::OnImageFetched,
                   base::Unretained(this)));
  }

  MOCK_METHOD1(OnImageFetched, void(const gfx::Image&));

  RemoteSuggestionsDatabase* database() { return database_.get(); }
  net::FakeURLFetcherFactory* fake_url_fetcher_factory() {
    return fake_url_fetcher_factory_.get();
  }
  MockImageDecoder* mock_image_decoder() { return mock_image_decoder_; }

 private:
  std::unique_ptr<CachedImageFetcher> cached_image_fetcher_;
  std::unique_ptr<RemoteSuggestionsDatabase> database_;
  base::ScopedTempDir database_dir_;
  std::unique_ptr<net::FakeURLFetcherFactory> fake_url_fetcher_factory_;
  MockImageDecoder* mock_image_decoder_;
  scoped_refptr<base::TestMockTimeTaskRunner> mock_task_runner_;
  base::ThreadTaskRunnerHandle mock_task_runner_handle_;
  std::unique_ptr<TestingPrefServiceSimple> pref_service_;
  scoped_refptr<net::TestURLRequestContextGetter> request_context_getter_;

  DISALLOW_COPY_AND_ASSIGN(CachedImageFetcherTest);
};

TEST_F(CachedImageFetcherTest, DatabaseIsInitialized) {
  EXPECT_TRUE(database()->IsInitialized());
}

TEST_F(CachedImageFetcherTest, FetchImageFromCache) {
  // Save the image in the database
  database()->SaveImage(kSnippetID, kImageData);
  FastForwardUntilNoTasksRemain();

  // Expect that URL is not fetched because image is in cache
  EXPECT_CALL(*mock_image_decoder(), DecodeImage(kImageData, _, _));
  FetchImage();
  FastForwardUntilNoTasksRemain();
}

TEST_F(CachedImageFetcherTest, FetchImageNotInCache) {
  fake_url_fetcher_factory()->SetFakeResponse(
      kImageURL, kImageData, net::HTTP_OK, net::URLRequestStatus::SUCCESS);
  EXPECT_CALL(*mock_image_decoder(), DecodeImage(kImageData, _, _));
  FetchImage();
  FastForwardUntilNoTasksRemain();
}

TEST_F(CachedImageFetcherTest, FetchNonExistingImage) {
  const std::string kErrorResponse = "error-response";
  fake_url_fetcher_factory()->SetFakeResponse(kImageURL, kErrorResponse,
                                              net::HTTP_NOT_FOUND,
                                              net::URLRequestStatus::FAILED);
  // Expect that decoder is called even if URL cannot be requested,
  // then with empty image data
  const std::string kEmptyImageData;
  EXPECT_CALL(*mock_image_decoder(), DecodeImage(kEmptyImageData, _, _));
  FetchImage();
  FastForwardUntilNoTasksRemain();
}

}  // namespace ntp_snippets
