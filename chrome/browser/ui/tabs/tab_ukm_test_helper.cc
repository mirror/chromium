// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/tabs/tab_ukm_test_helper.h"

#include "chrome/test/base/testing_profile.h"
#include "components/ukm/ukm_source.h"
#include "content/public/test/web_contents_tester.h"
#include "services/metrics/public/interfaces/ukm_interface.mojom.h"

using content::WebContentsTester;

TestWebContentsObserver::TestWebContentsObserver(
    content::WebContents* web_contents)
    : content::WebContentsObserver(web_contents) {}

void TestWebContentsObserver::WebContentsDestroyed() {
  // Verify that a WebContents being destroyed does not trigger logging.
  web_contents()->WasHidden();
}

TabActivityTestBase::TabActivityTestBase() = default;
TabActivityTestBase::~TabActivityTestBase() = default;

void TabActivityTestBase::TearDown() {
  ChromeRenderViewHostTestHarness::TearDown();
}

content::WebContents* TabActivityTestBase::AddWebContentsAndNavigate(
    TabStripModel* tab_strip_model,
    const GURL& initial_url) {
  content::WebContents::CreateParams params(profile(), nullptr);
  // Create as a background tab if there are other tabs in the tab strip.
  params.initially_hidden = tab_strip_model->count() > 0;
  content::WebContents* test_contents =
      WebContentsTester::CreateTestWebContents(params);

  // Create the TestWebContentsObserver to observe |test_contents|. When the
  // WebContents is destroyed, the observer will be reset automatically.
  observers_.push_back(
      std::make_unique<TestWebContentsObserver>(test_contents));

  tab_strip_model->AppendWebContents(test_contents, false);
  WebContentsTester::For(test_contents)->NavigateAndCommit(initial_url);
  return test_contents;
}

void TabActivityTestBase::SwitchToTabAt(TabStripModel* tab_strip_model,
                                        int new_index) {
  int active_index = tab_strip_model->active_index();
  EXPECT_NE(new_index, active_index);

  content::WebContents* active_contents =
      tab_strip_model->GetWebContentsAt(active_index);
  ASSERT_TRUE(active_contents);
  content::WebContents* new_contents =
      tab_strip_model->GetWebContentsAt(new_index);
  ASSERT_TRUE(new_contents);

  // Activate the tab. Normally this would hide the active tab's aura::Window,
  // which is what actually triggers TabActivityWatcher to log the change. For
  // a TestWebContents, we must manually call WasHidden(), and do the reverse
  // for the newly activated tab.
  tab_strip_model->ActivateTabAt(new_index, /*user_gesture=*/true);
  active_contents->WasHidden();
  new_contents->WasShown();
}

UkmEntryChecker::UkmEntryChecker() = default;
UkmEntryChecker::~UkmEntryChecker() = default;

void UkmEntryChecker::ExpectNewEntry(const char* entry_name,
                                     const GURL& source_url,
                                     const UkmMetricMap& expected_metrics) {
  num_entries_[entry_name]++;  // There should only be 1 more entry than before.
  std::vector<const ukm::mojom::UkmEntry*> entries =
      ukm_recorder_.GetEntriesByName(entry_name);
  ASSERT_EQ(NumEntries(entry_name), entries.size());

  // Verify the entry is associated with the correct URL.
  const ukm::mojom::UkmEntry* entry = entries.back();
  if (!source_url.is_empty())
    ukm_recorder_.ExpectEntrySourceHasUrl(entry, source_url);

  // Each expected metric should match a named value in the UKM entry.
  for (const std::pair<const char*, uint64_t>& pair : expected_metrics)
    ukm::TestUkmRecorder::ExpectEntryMetric(entry, pair.first, pair.second);
}

void UkmEntryChecker::ExpectNewEntries(
    const char* entry_name,
    const SourceUkmMetricMap& expected_data) {
  std::vector<const ukm::mojom::UkmEntry*> entries =
      ukm_recorder_.GetEntriesByName(entry_name);

  const size_t num_new_entries = expected_data.size();
  const size_t num_entries = entries.size();
  num_entries_[entry_name] += num_new_entries;

  ASSERT_EQ(NumEntries(entry_name), entries.size());
  std::set<ukm::SourceId> found_source_ids;

  for (size_t i = 0; i < num_new_entries; ++i) {
    const ukm::mojom::UkmEntry* entry =
        entries[num_entries - num_new_entries + i];
    const ukm::SourceId& source_id = entry->source_id;
    const auto& expected_data_for_id = expected_data.find(source_id);
    EXPECT_TRUE(expected_data_for_id != expected_data.end());
    EXPECT_TRUE(found_source_ids.find(source_id) == found_source_ids.end());

    found_source_ids.insert(source_id);
    const std::pair<GURL, UkmMetricMap>& expected_url_metrics =
        expected_data_for_id->second;

    const GURL& source_url = expected_url_metrics.first;
    const UkmMetricMap& expected_metrics = expected_url_metrics.second;
    if (!source_url.is_empty())
      ukm_recorder_.ExpectEntrySourceHasUrl(entry, source_url);

    // Each expected metric should match a named value in the UKM entry.
    for (const std::pair<const char*, uint64_t>& pair : expected_metrics)
      ukm::TestUkmRecorder::ExpectEntryMetric(entry, pair.first, pair.second);
  }
}

bool UkmEntryChecker::WasNewEntryRecorded(const char* entry_name,
                                          size_t new_num_entries) const {
  return ukm_recorder_.GetEntriesByName(entry_name).size() ==
         NumEntries(entry_name) + new_num_entries;
}

size_t UkmEntryChecker::NumEntries(const char* entry_name) const {
  if (num_entries_.find(entry_name) == num_entries_.end()) {
    return 0;
  }
  return num_entries_.find(entry_name)->second;
}

const ukm::mojom::UkmEntry* UkmEntryChecker::LastUkmEntry(
    const char* entry_name) const {
  std::vector<const ukm::mojom::UkmEntry*> entries =
      ukm_recorder_.GetEntriesByName(entry_name);
  EXPECT_FALSE(entries.empty());
  return entries.back();
}

ukm::SourceId UkmEntryChecker::GetSoureIdForUrl(const GURL& source_url) {
  const ukm::UkmSource* source = ukm_recorder_.GetSourceForUrl(source_url);
  EXPECT_TRUE(source);
  return source->id();
}
