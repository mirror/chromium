// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/strings/utf_string_conversions.h"
#include "base/test/scoped_task_environment.h"
#include "components/omnibox/browser/fake_autocomplete_provider_client.h"
#include "components/omnibox/browser/history_quick_provider.h"
#include "testing/gtest/include/gtest/gtest.h"

class FakeHistoryProvider : public HistoryProvider {
 public:
  explicit FakeHistoryProvider(AutocompleteProviderClient* client)
      : HistoryProvider(AutocompleteProvider::TYPE_HISTORY_QUICK, client) {}

  void Start(const AutocompleteInput& input, bool minimal_changes) override;

 private:
  ~FakeHistoryProvider() override;

  DISALLOW_COPY_AND_ASSIGN(FakeHistoryProvider);
};

void FakeHistoryProvider::Start(const AutocompleteInput& input,
                                bool minimal_changes) {}

FakeHistoryProvider::~FakeHistoryProvider() {}

class HistoryProviderTest : public testing::Test {
 public:
  HistoryProviderTest() = default;

 protected:
  void SetUp() override;
  void TearDown() override;

  FakeAutocompleteProviderClient& client() { return *client_; }
  HistoryProvider& provider() { return *provider_; }

 private:
  base::test::ScopedTaskEnvironment scoped_task_environment_;
  std::unique_ptr<FakeAutocompleteProviderClient> client_;

  ACMatches ac_matches_;  // The resulting matches after running RunTest.

  scoped_refptr<FakeHistoryProvider> provider_;

  DISALLOW_COPY_AND_ASSIGN(HistoryProviderTest);
};

void HistoryProviderTest::SetUp() {
  client_.reset(new FakeAutocompleteProviderClient());
  provider_ = new FakeHistoryProvider(client_.get());
}

void HistoryProviderTest::TearDown() {
  provider_ = nullptr;
  // The InMemoryURLIndex must be explicitly shut down or it will DCHECK() in
  // its destructor.
  client_->GetInMemoryURLIndex()->Shutdown();
  client_->set_in_memory_url_index(nullptr);
  // History index rebuild task is created from main thread during SetUp,
  // performed on DB thread and must be deleted on main thread.
  // Run main loop to process delete task, to prevent leaks.
  base::RunLoop().RunUntilIdle();
}

TEST_F(HistoryProviderTest, ConvertsOpenTabsCorrectly) {
  AutocompleteMatch match;
  match.contents = base::UTF8ToUTF16("some-url.com");
  provider().matches_.push_back(match);
  match.contents = base::UTF8ToUTF16("some-other-url.com");
  match.description = base::UTF8ToUTF16("Some Other Site");
  provider().matches_.push_back(match);

  // Have IsTabOpenWithURL() return true;
  client().set_is_tab_open_with_url(true);

  provider().ConvertOpenTabMatches();

  EXPECT_EQ(base::UTF8ToUTF16("Switch to tab"),
            provider().matches_[0].description);
  EXPECT_EQ(base::UTF8ToUTF16("Switch to tab - Some Other Site"),
            provider().matches_[1].description);
}
