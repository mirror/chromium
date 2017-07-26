// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/prefetch/mock_prefetch_item_generator.h"

#include "base/strings/string_number_conversions.h"
#include "base/time/time.h"
#include "components/offline_pages/core/client_id.h"
#include "components/offline_pages/core/prefetch/store/prefetch_store_utils.h"
#include "url/gurl.h"

namespace offline_pages {

// All static.
const std::string MockPrefetchItemGenerator::kClientNamespace =
    std::string("test_prefetch_namespace");
const std::string MockPrefetchItemGenerator::kGuidPrefix =
    std::string("test_guid_");
const std::string MockPrefetchItemGenerator::kClientIdPrefix =
    std::string("test_client_id_");
const std::string MockPrefetchItemGenerator::kUrlPrefix =
    std::string("http://www.requested.com/");
const std::string MockPrefetchItemGenerator::kFinalUrlPrefix =
    std::string("http://www.final.com/");
const std::string MockPrefetchItemGenerator::kOperationNamePrefix =
    std::string("test_operation_name_");
const std::string MockPrefetchItemGenerator::kArchiveBodyNamePrefix =
    std::string("test_archive_body_name_");

MockPrefetchItemGenerator::MockPrefetchItemGenerator() = default;

MockPrefetchItemGenerator::~MockPrefetchItemGenerator() = default;

PrefetchItem MockPrefetchItemGenerator::CreateItem(PrefetchItemState state) {
  static int item_counter = 0;
  PrefetchItem new_item;
  new_item.state = state;

  // Values always set using prefixes.
  new_item.guid = guid_prefix_ + base::IntToString(item_counter);
  new_item.client_id = ClientId(
      client_namespace_, client_id_prefix_ + base::IntToString(item_counter));
  new_item.url = GURL(url_prefix_ + base::IntToString(item_counter));

  // Values set only if prefixes are not empty.
  if (final_url_prefix_.length())
    new_item.final_archived_url =
        GURL(final_url_prefix_ + base::IntToString(item_counter));
  if (operation_name_prefix_.length())
    new_item.operation_name =
        operation_name_prefix_ + base::IntToString(item_counter);
  if (archive_body_name_prefix_.length()) {
    new_item.archive_body_name =
        archive_body_name_prefix_ + base::IntToString(item_counter);
    new_item.archive_body_length = item_counter * 100;
  }

  // Values set with non prefix based values.
  new_item.offline_id = GenerateOfflineId();
  new_item.creation_time = base::Time::Now();
  new_item.freshness_time = new_item.creation_time;

  ++item_counter;
  return new_item;
}

}  // namespace offline_pages
